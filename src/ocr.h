#pragma once

#include <QImage>
#include <QList>
#include <QRect>
#include <QString>

struct OcrLine { QRect box; QString text; float score = 0; };

// RapidOCR(PP-OCRv4) 离线文字识别：ONNX 检测 + 识别，词典内嵌于识别模型元数据。
// 模型目录约定：ch_PP-OCRv4_det_infer.onnx、ch_PP-OCRv4_rec_infer.onnx。
class Ocr {
public:
    Ocr();
    ~Ocr();
    Ocr(const Ocr &) = delete;
    Ocr &operator=(const Ocr &) = delete;

    bool ensureLoaded(const QString &modelDir);
    bool isLoaded() const;
    QString lastError() const;
    QList<OcrLine> run(const QImage &image);

private:
    struct Impl;
    Impl *impl = nullptr;
    QString lastError_;
};
