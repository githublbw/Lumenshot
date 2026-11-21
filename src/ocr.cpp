#include "ocr.h"

#include <QFile>
#include <QPainter>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <onnxruntime_cxx_api.h>
#include <thread>

struct Ocr::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_ERROR, "lumenshot-ocr"};
    std::unique_ptr<Ort::Session> det;
    std::unique_ptr<Ort::Session> rec;
    QStringList dict;      // 识别模型元数据内嵌词典，下标 0 为 blank，末尾追加空格
};

namespace {

constexpr int detLimitSide = 736;      // RapidOCR 默认：短边放大到 736，利于小字召回
constexpr float detBinThreshold = 0.3f;
constexpr float detBoxScore = 0.5f;
constexpr float detUnclipRatio = 1.6f;
constexpr int recHeight = 48;
constexpr int recMaxWidth = 640;
constexpr float recKeepScore = 0.3f;    // 片段级：仅过滤明显噪声
constexpr float recRowScore = 0.5f;     // 行级：聚合后再过滤，与官方 text_score 一致

// 检测输入：短边不足 736 时放大，宽高对齐 32，不足处补黑。
void prepareDetInput(const QImage &source, QImage &resized, qreal &ratio)
{
    const int minSide = std::min(source.width(), source.height());
    ratio = minSide < detLimitSide ? qreal(detLimitSide) / minSide : 1.0;
    const int rw = std::max(32, int(source.width() * ratio));
    const int rh = std::max(32, int(source.height() * ratio));
    const int alignedW = (rw + 31) / 32 * 32;
    const int alignedH = (rh + 31) / 32 * 32;
    resized = QImage(alignedW, alignedH, QImage::Format_RGB888);
    resized.fill(Qt::black);
    QPainter painter(&resized);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRect(0, 0, rw, rh), source);
}

// 阈值化 + 连通域：屏幕文字均为横平竖直，直接取外接矩形，无需透视矫正。
QList<QRect> findTextRegions(const float *prob, int width, int height)
{
    // 官方 use_dilation=True：先对二值图做一次 3x3 膨胀再找连通域。
    std::vector<uint8_t> binary(size_t(width) * height, 0);
    for (int i = 0; i < width * height; ++i) binary[i] = prob[i] >= detBinThreshold;
    std::vector<uint8_t> dilated(size_t(width) * height, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool on = false;
            for (int dy = -1; dy <= 1 && !on; ++dy) {
                for (int dx = -1; dx <= 1 && !on; ++dx) {
                    const int nx = x + dx, ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                    on = binary[ny * width + nx];
                }
            }
            dilated[y * width + x] = on;
        }
    }
    std::vector<uint8_t> visited(size_t(width) * height, 0);
    QList<QRect> regions;
    std::vector<int> queue;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int index = y * width + x;
            if (visited[index] || !dilated[index]) continue;
            visited[index] = 1;
            queue.clear();
            queue.push_back(index);
            int left = x, right = x, top = y, bottom = y, count = 0;
            float sum = 0;
            for (size_t head = 0; head < queue.size(); ++head) {
                const int current = queue[head];
                const int cx = current % width, cy = current / width;
                ++count;
                sum += prob[current];
                left = std::min(left, cx);
                right = std::max(right, cx);
                top = std::min(top, cy);
                bottom = std::max(bottom, cy);
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = cx + dx, ny = cy + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                        const int neighbor = ny * width + nx;
                        if (visited[neighbor] || !dilated[neighbor]) continue;
                        visited[neighbor] = 1;
                        queue.push_back(neighbor);
                    }
                }
            }
            const int w = right - left + 1, h = bottom - top + 1;
            if (count < 8 || w < 2 || h < 2) continue;
            if (sum / count < detBoxScore) continue;
            // 近似 DB unclip：按面积/周长比外扩，避免裁掉笔画边缘。
            const int grow = std::clamp(int(count * detUnclipRatio / (2 * (w + h))) + 1, 1, 32);
            regions.append(QRect(left - grow, top - grow, w + grow * 2, h + grow * 2)
                               .intersected(QRect(0, 0, width, height)));
        }
    }
    return regions;
}

// 先按行聚合再按起点排序，保证阅读顺序。
QList<QRect> orderRegions(QList<QRect> regions)
{
    std::sort(regions.begin(), regions.end(), [](const QRect &a, const QRect &b) {
        return a.top() < b.top();
    });
    QList<QRect> ordered;
    QList<QRect> row;
    const auto flushRow = [&]{
        std::sort(row.begin(), row.end(), [](const QRect &a, const QRect &b) {
            return a.left() < b.left();
        });
        ordered.append(row);
        row.clear();
    };
    int rowBottom = std::numeric_limits<int>::min();
    for (const QRect &box : regions) {
        if (!row.isEmpty() && box.top() > rowBottom - box.height() / 2) flushRow();
        row.append(box);
        rowBottom = std::max(rowBottom, box.bottom());
    }
    if (!row.isEmpty()) flushRow();
    return ordered;
}

std::vector<float> recInput(const QImage &crop, int &width)
{
    const qreal scale = qreal(recHeight) / crop.height();
    width = std::clamp(int(crop.width() * scale), 8, recMaxWidth);
    const QImage input = crop.scaled(width, recHeight, Qt::IgnoreAspectRatio,
                                     Qt::SmoothTransformation).convertToFormat(QImage::Format_BGR888);
    std::vector<float> data(size_t(width) * recHeight * 3);
    for (int y = 0; y < recHeight; ++y) {
        const uchar *line = input.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            const size_t base = size_t(y) * width + x;
            for (int c = 0; c < 3; ++c)
                data[base + size_t(c) * recHeight * width] = line[x * 3 + c] / 127.5f - 1.0f;
        }
    }
    return data;
}

struct Decoded { QString text; float score = 0; };

Decoded decodeCtc(const float *probs, int steps, int classes, const QStringList &dict)
{
    Decoded result;
    float sum = 0;
    int kept = 0;
    int previous = 0;
    for (int t = 0; t < steps; ++t) {
        const float *row = probs + size_t(t) * classes;
        int best = 0;
        float bestProb = row[0];
        for (int c = 1; c < classes; ++c) {
            if (row[c] > bestProb) { bestProb = row[c]; best = c; }
        }
        if (best != 0 && best != previous) {
            result.text += best <= dict.size() ? dict.at(best - 1) : QStringLiteral(" ");
            sum += bestProb;
            ++kept;
        }
        previous = best;
    }
    if (kept > 0) result.score = sum / kept;
    return result;
}

// 版面重组：检测框是“文字片段”粒度，按垂直重叠聚合成视觉行，再左→右拼接。
QList<OcrLine> mergeRows(QList<OcrLine> lines)
{
    std::sort(lines.begin(), lines.end(), [](const OcrLine &a, const OcrLine &b) {
        return a.box.center().y() < b.box.center().y();
    });
    QList<QList<OcrLine>> rows;
    QList<OcrLine> row;
    int rowTop = 0, rowBottom = 0;
    const auto flushRow = [&]{
        std::sort(row.begin(), row.end(), [](const OcrLine &a, const OcrLine &b) {
            return a.box.left() < b.box.left();
        });
        rows.append(row);
        row.clear();
    };
    for (const OcrLine &line : lines) {
        if (!row.isEmpty()) {
            const int rowMid = (rowTop + rowBottom) / 2;
            const int lineMid = line.box.center().y();
            const int tolerance = std::max(rowBottom - rowTop, line.box.height()) * 3 / 5;
            if (qAbs(lineMid - rowMid) > tolerance) flushRow();
        }
        if (row.isEmpty()) {
            rowTop = line.box.top();
            rowBottom = line.box.bottom();
        } else {
            rowTop = std::min(rowTop, line.box.top());
            rowBottom = std::max(rowBottom, line.box.bottom());
        }
        row.append(line);
    }
    if (!row.isEmpty()) flushRow();

    QList<OcrLine> merged;
    for (const QList<OcrLine> &oneRow : rows) {
        QString text;
        int prevRight = oneRow.first().box.left();
        int rowH = 1;
        for (const OcrLine &item : oneRow) rowH = std::max(rowH, item.box.height());
        QRect bounds = oneRow.first().box;
        float scoreSum = 0;
        int weightSum = 0;
        for (int i = 0; i < oneRow.size(); ++i) {
            if (i > 0) {
                const int gap = oneRow.at(i).box.left() - prevRight;
                if (gap > rowH * 5 / 2) text += QStringLiteral("  ");   // 栏间距
                else if (gap > rowH * 2 / 5) text += QLatin1Char(' ');  // 词间距
            }
            text += oneRow.at(i).text;
            prevRight = oneRow.at(i).box.right();
            bounds = bounds.united(oneRow.at(i).box);
            const int weight = std::max(1, int(oneRow.at(i).text.size()));
            scoreSum += oneRow.at(i).score * weight;
            weightSum += weight;
        }
        const float score = scoreSum / std::max(1, weightSum);
        if (score >= recRowScore) merged.append({bounds, text, score});
    }
    return merged;
}

} // namespace

Ocr::Ocr() = default;
Ocr::~Ocr() { delete impl; }

bool Ocr::ensureLoaded(const QString &modelDir)
{
    if (impl) return true;
    const QString detPath = modelDir + "/ch_PP-OCRv4_det_infer.onnx";
    const QString recPath = modelDir + "/ch_PP-OCRv4_rec_infer.onnx";
    if (!QFile::exists(detPath) || !QFile::exists(recPath)) {
        lastError_ = QStringLiteral("缺少 OCR 模型文件（%1）").arg(modelDir);
        return false;
    }
    try {
        auto *next = new Impl;
        Ort::SessionOptions options;
        const int threads = std::clamp(int(std::thread::hardware_concurrency()), 2, 8);
        options.SetIntraOpNumThreads(threads);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        next->det = std::make_unique<Ort::Session>(next->env, detPath.toStdString().c_str(), options);
        next->rec = std::make_unique<Ort::Session>(next->env, recPath.toStdString().c_str(), options);
        Ort::AllocatorWithDefaultOptions allocator;
        auto characters = next->rec->GetModelMetadata().LookupCustomMetadataMapAllocated("character", allocator);
        if (!characters) {
            lastError_ = QStringLiteral("识别模型缺少内嵌词典");
            delete next;
            return false;
        }
        next->dict = QString::fromUtf8(characters.get()).split('\n', Qt::SkipEmptyParts);
        allocator.Free(characters.release());
        impl = next;
        return true;
    } catch (const Ort::Exception &exception) {
        lastError_ = QStringLiteral("OCR 引擎加载失败：") + exception.what();
        return false;
    }
}

bool Ocr::isLoaded() const { return impl != nullptr; }

QString Ocr::lastError() const { return lastError_; }

QList<OcrLine> Ocr::run(const QImage &image)
{
    if (!impl || image.isNull()) return {};
    const bool debug = qEnvironmentVariableIsSet("OCR_DEBUG");
    QList<OcrLine> output;
    try {
        const QImage source = image.convertToFormat(QImage::Format_BGR888);
        QImage detInput;
        qreal ratio = 1.0;
        prepareDetInput(source, detInput, ratio);

        Ort::AllocatorWithDefaultOptions allocator;
        auto detIn = impl->det->GetInputNameAllocated(0, allocator);
        auto detOut = impl->det->GetOutputNameAllocated(0, allocator);
        const int w = detInput.width(), h = detInput.height();
        std::vector<float> detData(size_t(w) * h * 3);
        for (int y = 0; y < h; ++y) {
            const uchar *line = detInput.constScanLine(y);
            for (int x = 0; x < w; ++x) {
                const size_t base = size_t(y) * w + x;
                for (int c = 0; c < 3; ++c)
                    detData[base + size_t(c) * w * h] = line[x * 3 + c] / 127.5f - 1.0f;
            }
        }
        const int64_t detShape[4] = {1, 3, h, w};
        const Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value detTensor = Ort::Value::CreateTensor<float>(
            memory, detData.data(), detData.size(), detShape, 4);
        const char *detInputs[] = {detIn.get()};
        const char *detOutputs[] = {detOut.get()};
        auto detResult = impl->det->Run(Ort::RunOptions{nullptr},
                                        detInputs, &detTensor, 1, detOutputs, 1);
        const float *prob = detResult.front().GetTensorData<float>();
        const auto probShapeInfo = detResult.front().GetTensorTypeAndShapeInfo();
        const auto probShape = probShapeInfo.GetShape();
        const int pw = int(probShape.back()), ph = int(probShape.at(probShape.size() - 2));

        QList<QRect> regions = orderRegions(findTextRegions(prob, pw, ph));
        if (debug) {
            source.save("/tmp/ocr-debug-input.png");
            FILE *f = fopen("/tmp/ocr-debug.log", "w");
            fprintf(f, "regions=%d input=%dx%d\n", regions.size(), pw, ph);
            for (const QRect &b : regions)
                fprintf(f, "  region (%d,%d %dx%d)\n", b.x(), b.y(), b.width(), b.height());
            fclose(f);
        }
        for (QRect &box : regions) {
            box = QRect(qRound(box.x() / ratio), qRound(box.y() / ratio),
                        qRound(box.width() / ratio), qRound(box.height() / ratio))
                      .intersected(source.rect());
        }

        auto recIn = impl->rec->GetInputNameAllocated(0, allocator);
        auto recOut = impl->rec->GetOutputNameAllocated(0, allocator);
        const char *recInputs[] = {recIn.get()};
        const char *recOutputs[] = {recOut.get()};
        const auto runRec = [&](const QImage &crop) -> Decoded {
            if (crop.isNull() || crop.width() < 2 || crop.height() < 2) return {};
            int width = 0;
            std::vector<float> recData = recInput(crop, width);
            const int64_t recShape[4] = {1, 3, recHeight, width};
            Ort::Value recTensor = Ort::Value::CreateTensor<float>(
                memory, recData.data(), recData.size(), recShape, 4);
            auto recResult = impl->rec->Run(Ort::RunOptions{nullptr},
                                            recInputs, &recTensor, 1, recOutputs, 1);
            const float *probs = recResult.front().GetTensorData<float>();
            const auto outShapeInfo = recResult.front().GetTensorTypeAndShapeInfo();
            const auto outShape = outShapeInfo.GetShape();
            return decodeCtc(probs, int(outShape.at(1)), int(outShape.at(2)), impl->dict);
        };
        for (const QRect &box : regions) {
            const Decoded decoded = runRec(source.copy(box));
            // 标点等小片段单独成框时置信度偏低，先保留，待同行聚合后按整行分数过滤。
            if (debug)
                fprintf(stderr, "[ocr] fragment box=(%d,%d %dx%d) score=%.3f text=%s\n",
                        box.x(), box.y(), box.width(), box.height(), decoded.score,
                        decoded.text.toUtf8().constData());
            if (decoded.text.trimmed().isEmpty() || decoded.score < recKeepScore) continue;
            output.append({box, decoded.text, decoded.score});
        }
        QList<OcrLine> rows = mergeRows(output);
        // 行级复核：检测碎片可能丢掉冒号、箭头等小字符，对整行裁剪再识别一次；
        // 整行结果更完整（字符更多且分数达标）时优先采用。
        QList<OcrLine> refined;
        for (const OcrLine &row : rows) {
            const Decoded whole = runRec(source.copy(row.box));
            if (debug)
                fprintf(stderr, "[ocr] row box=(%d,%d %dx%d) concat=%s(%.3f) whole=%s(%.3f)\n",
                        row.box.x(), row.box.y(), row.box.width(), row.box.height(),
                        row.text.toUtf8().constData(), row.score,
                        whole.text.toUtf8().constData(), whole.score);
            if (whole.score >= recRowScore && whole.text.size() > row.text.size())
                refined.append({row.box, whole.text, whole.score});
            else
                refined.append(row);
        }
        return refined;
    } catch (const Ort::Exception &exception) {
        lastError_ = QStringLiteral("OCR 推理失败：") + exception.what();
        return {};
    }
}
