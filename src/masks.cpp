#include "masks.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetricsF>

void Masks::reset(QImage image)
{
    original = image.convertToFormat(QImage::Format_ARGB32);
    original.setDevicePixelRatio(1);
    output = original;
    steps.clear();
    count = 0;
}

void Masks::apply(const Step &step)
{
    if (step.shape == MaskShape::Blur || step.shape == MaskShape::Mosaic) {
        auto region = output.copy(step.pixels);
        const QSize grid(std::max(1, (region.width()+step.size-1)/step.size),
                         std::max(1, (region.height()+step.size-1)/step.size));
        region = region.scaled(grid, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPainter p(&output);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.setRenderHint(QPainter::SmoothPixmapTransform, step.shape == MaskShape::Blur);
        p.drawImage(step.pixels, region);
        return;
    }
    if (step.shape == MaskShape::Pen) {
        if (step.points.size() >= 2) {
            QPainter p(&output);
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.moveTo(step.points.first());
            for (int i = 1; i < step.points.size(); ++i) path.lineTo(step.points.at(i));
            p.setPen(QPen(step.color, std::max(1, step.size), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        }
        return;
    }
    if (step.shape == MaskShape::MosaicBrush) {
        if (step.points.size() >= 2) {
            QPainterPath strokePath;
            strokePath.moveTo(step.points.first());
            for (int i = 1; i < step.points.size(); ++i) strokePath.lineTo(step.points.at(i));
            QPainterPathStroker stroker;
            stroker.setWidth(std::max(4, step.size));
            stroker.setCapStyle(Qt::RoundCap);
            stroker.setJoinStyle(Qt::RoundJoin);
            QPainterPath clip = stroker.createStroke(strokePath);
            // 小间距采样点会让描边轮廓大量自重叠，OddEven 规则会把重叠区抵消成洞，
            // 改用 WindingFill 保证笔刷路径连续覆盖。
            clip.setFillRule(Qt::WindingFill);
            QRect region = clip.boundingRect().toAlignedRect().intersected(output.rect());
            if (!region.isEmpty()) {
                QImage img = output.copy(region);
                const int block = std::clamp(step.size, 6, 40);
                QImage small = img.scaled(std::max(1, region.width() / block),
                                          std::max(1, region.height() / block),
                                          Qt::IgnoreAspectRatio, Qt::FastTransformation)
                                   .convertToFormat(QImage::Format_ARGB32);
                // 每个像素块加确定性亮度抖动：纯色背景上也能看出连续的笔刷轨迹
                const auto jitterFor = [](int bx, int by) {
                    const int h = (bx * 73856093) ^ (by * 19349663);
                    return ((h % 25) + 25) % 25 - 12;
                };
                for (int by = 0; by < small.height(); ++by) {
                    for (int bx = 0; bx < small.width(); ++bx) {
                        const QColor c = small.pixelColor(bx, by);
                        const int j = jitterFor(bx, by);
                        small.setPixel(bx, by, qRgb(std::clamp(c.red() + j, 0, 255),
                                                    std::clamp(c.green() + j, 0, 255),
                                                    std::clamp(c.blue() + j, 0, 255)));
                    }
                }
                QImage pixelated = small.scaled(region.width(), region.height(),
                                                Qt::IgnoreAspectRatio, Qt::FastTransformation);
                QPainter p(&output);
                p.setClipPath(clip);
                p.drawImage(region.topLeft(), pixelated);
            }
        }
        return;
    }
    if (step.shape == MaskShape::RoundedRect) {
        QPainter p(&output);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(step.pixels);
        const qreal rad = std::clamp(std::min(r.width(), r.height()) * 0.14, 4.0, 18.0);
        QColor border = step.color;
        border.setAlpha(255);
        p.setPen(QPen(border, 1.6));
        p.setBrush(step.color);
        p.drawRoundedRect(r, rad, rad);
        return;
    }
    if (step.shape == MaskShape::Bubble) {
        QPainter p(&output);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(step.pixels);
        const qreal rad = std::clamp(std::min(r.width(), r.height()) * 0.12, 4.0, 16.0);
        QPainterPath bubble;
        bubble.addRoundedRect(r, rad, rad);
        // 尾巴指向拖动起点（说话者方向）
        const QPointF target = step.line.p1();
        const QPointF center = r.center();
        const QPointF delta = target - center;
        if (!r.contains(target)) {
            // 目标在气泡外：尾巴从边界指向目标
            QPointF wing(-delta.y(), delta.x());
            const qreal wingLen = std::hypot(wing.x(), wing.y());
            if (wingLen > 1e-6) wing /= wingLen;
            qreal t = 1.0;
            if (delta.x() != 0) t = std::min(t, (r.width() / 2 + 1) / std::abs(delta.x()));
            if (delta.y() != 0) t = std::min(t, (r.height() / 2 + 1) / std::abs(delta.y()));
            const QPointF edge = center + delta * t;
            bubble.moveTo(edge + wing * 8);
            bubble.lineTo(target);
            bubble.lineTo(edge - wing * 8);
        } else {
            // 拖动起点是气泡自身的角：尾巴沿对角向外伸出，指向说话者方向
            const qreal len = std::hypot(delta.x(), delta.y());
            const QPointF outward = len > 1e-6 ? delta / len : QPointF(-0.707, -0.707);
            const QPointF apex = target + outward * 12;
            bubble.moveTo(target + QPointF(delta.x() > 0 ? -8 : 8, 0));
            bubble.lineTo(apex);
            bubble.lineTo(target + QPointF(0, delta.y() > 0 ? -8 : 8));
        }
        p.setPen(QPen(step.color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(step.color);
        p.drawPath(bubble);
        return;
    }
    if (step.shape == MaskShape::Number) {
        // 无底色描边数字：彩色数字 + 白色描边，任何背景都清晰且不遮挡内容。
        QPainter p(&output);
        p.setRenderHint(QPainter::Antialiasing);
        QFont font;
        font.setPixelSize(qMax(8, qRound(step.size * 0.56)));
        font.setBold(true);
        p.setFont(font);
        const QPointF center = step.pixels.center();
        QPainterPath glyph;
        glyph.addText(center, font, step.text);
        glyph.translate(center - glyph.boundingRect().center());
        p.setPen(QPen(QColor(255, 255, 255, 235), std::max(2.0, step.size * 0.14),
                      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(glyph);
        p.setPen(Qt::NoPen);
        p.setBrush(step.color);
        p.drawPath(glyph);
        return;
    }
    QPainter painter(&output);
    painter.setPen(Qt::NoPen);
    painter.setBrush(step.color);
    if (step.shape == MaskShape::Rectangle)
        painter.fillRect(step.pixels, step.color);
    else if (step.shape == MaskShape::Ellipse)
        painter.drawEllipse(step.pixels);
    else if (step.shape == MaskShape::Arrow) {
        // 极简细线箭头：更细的杆 + 小巧紧致的三角头，头随长度按比例收缩。
        const auto line = step.line;
        const qreal width = std::max(1, step.size);
        const QPointF delta = line.p2() - line.p1();
        const qreal length = std::hypot(delta.x(), delta.y());
        if (length > 0.5) {
            const QPointF direction = delta / length;
            const QPointF wing(-direction.y(), direction.x());
            const qreal head = std::clamp(length * 0.17, width * 3.5,
                                          std::min(length * 0.28, width * 11.0));
            const qreal half = head * 0.40;
            const QPointF base = line.p2() - direction * head;
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(step.color, width * 0.6, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(line.p1(), base);
            painter.setPen(Qt::NoPen);
            painter.setBrush(step.color);
            painter.drawPolygon(QPolygonF{line.p2(), base + wing * half, base - wing * half});
        }
    } else if (step.shape == MaskShape::Line || step.shape == MaskShape::Highlight) {
        const auto line = step.line;
        const QColor ink = step.shape == MaskShape::Highlight
            ? QColor(step.color.red(), step.color.green(), step.color.blue(), 140) : step.color;
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(ink, std::max(1, step.size), Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(line.p1(), line.p2());
    } else {
        QFont font;
        font.setPixelSize(step.size);
        painter.setFont(font);
        painter.setPen(step.color);
        painter.drawText(step.pixels, Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere, step.text);
    }
}

void Masks::add(MaskShape shape, const QPolygonF &points, QColor color, int size)
{
    if (points.size() < 2) return;
    if (shape != MaskShape::Pen && shape != MaskShape::MosaicBrush) return;
    QRectF bounds = points.boundingRect().adjusted(-qreal(size) / 2 - 2, -qreal(size) / 2 - 2,
                                                   qreal(size) / 2 + 2, qreal(size) / 2 + 2)
                         .intersected(QRectF(original.rect()));
    if (bounds.isEmpty()) return;
    steps.resize(count);
    steps.append({shape, bounds.toAlignedRect(), color, QLineF(),
                  std::clamp(size, 1, 60), QString(), points, 0});
    apply(steps.at(count++));
}

void Masks::add(MaskShape shape, QRectF bounds, QColor color, int size, QString text,
                int alphaOverride)
{
    const auto start = bounds.topLeft();
    const QLineF line(start, bounds.bottomRight());
    if (shape == MaskShape::Arrow || shape == MaskShape::Line || shape == MaskShape::Highlight) {
        if (line.length() < 1) return;
        const qreal pad = shape == MaskShape::Highlight ? std::max(3.0, size * 1.6)
                                                        : std::max(1.0, size * 0.8);
        bounds = bounds.normalized().adjusted(-pad, -pad, pad, pad);
    }
    if (shape == MaskShape::Number) {
        const qreal edge = std::clamp(size, 10, 96);
        bounds = QRectF(start - QPointF(edge / 2.0, edge / 2.0), QSizeF(edge, edge));
    }
    if (shape == MaskShape::Text) {
        if (text.trimmed().isEmpty() || !QRectF(original.rect()).contains(start)) return;
        const int wrap = std::clamp(420, 60, original.width() - int(start.x()));
        bounds = QRectF(start, QPointF(start.x() + wrap, original.height()));
    }
    bounds = bounds.normalized().intersected(original.rect());
    if (bounds.isEmpty()) return;
    if (shape == MaskShape::Highlight) color.setAlpha(140);
    else if (shape == MaskShape::Number) color.setAlpha(245);
    else if (alphaOverride >= 0 &&
             (shape == MaskShape::Rectangle || shape == MaskShape::Ellipse ||
              shape == MaskShape::RoundedRect || shape == MaskShape::Bubble))
        color.setAlpha(std::clamp(alphaOverride * 255 / 100, 20, 255));
    else color.setAlpha(255);
    steps.resize(count);
    steps.append({shape, bounds.toAlignedRect(), color, line, std::clamp(size, 1, 128), text,
                  QPolygonF(), shape == MaskShape::Text ? int(bounds.width()) : 0});
    apply(steps.at(count++));
}

void Masks::setStepSize(int index, int size)
{
    if (index < 0 || index >= count) return;
    Step &step = steps[index];
    switch (step.shape) {
    case MaskShape::Text:
    case MaskShape::Number:
        step.size = std::clamp(size, 8, 128);
        break;
    case MaskShape::Arrow:
    case MaskShape::Line:
    case MaskShape::Highlight:
    case MaskShape::Pen:
    case MaskShape::MosaicBrush:
        step.size = std::clamp(size, 1, 60);
        break;
    default:
        return;
    }
    rebuild();
}

void Masks::setStepAlpha(int index, int percent)
{
    if (index < 0 || index >= count) return;
    Step &step = steps[index];
    if (step.shape != MaskShape::Rectangle && step.shape != MaskShape::Ellipse &&
        step.shape != MaskShape::RoundedRect && step.shape != MaskShape::Bubble) return;
    step.color.setAlpha(std::clamp(percent * 255 / 100, 20, 255));
    rebuild();
}

void Masks::setStepTextWidth(int index, int width)
{
    if (index < 0 || index >= count) return;
    Step &step = steps[index];
    if (step.shape != MaskShape::Text) return;
    const int wrap = std::clamp(width, 60, original.width() - step.pixels.x());
    step.wrap = wrap;
    step.pixels = QRect(step.pixels.topLeft(), QSize(wrap, step.pixels.height()));
    rebuild();
}

void Masks::crop(const QRect &area)
{
    if (original.isNull()) return;
    const QRect keep = area.normalized().intersected(original.rect());
    if (keep.isEmpty() || keep == original.rect()) return;
    const QPoint offset = -keep.topLeft();
    original = original.copy(keep);
    output = output.copy(keep);
    QList<Step> kept;
    for (Step step : steps) {
        step.pixels.translate(offset);
        step.line.translate(offset);
        step.points.translate(offset);
        if (step.shape == MaskShape::Text) {
            step.pixels = step.pixels.intersected(output.rect());
            if (step.pixels.isEmpty()) continue;
        } else if (!step.pixels.intersects(output.rect())) {
            continue;
        }
        kept.append(step);
    }
    steps = kept;
    count = steps.size();
}

void Masks::rebuild()
{
    output = original;
    for (qsizetype i = 0; i < count; ++i) apply(steps.at(i));
}

QRectF Masks::textBounds(const Step &step) const
{
    QFont font;
    font.setPixelSize(std::max(1, step.size));
    const QFontMetricsF metrics(font);
    const int wrap = step.wrap > 0 ? step.wrap : step.pixels.width();
    const QRectF area(step.pixels.topLeft(), QSizeF(wrap, 100000));
    return metrics.boundingRect(area, Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere, step.text)
        .intersected(QRectF(original.rect()));
}

MaskShape Masks::stepShape(int index) const
{
    return index < 0 || index >= count ? MaskShape::Rectangle : steps.at(index).shape;
}

QRectF Masks::stepBounds(int index) const
{
    if (index < 0 || index >= count) return {};
    const Step &step = steps.at(index);
    if (step.shape == MaskShape::Text) return textBounds(step);
    if (step.shape == MaskShape::Pen || step.shape == MaskShape::MosaicBrush) {
        QRectF bounds(step.points.first(), step.points.first());
        for (const QPointF &pt : step.points) {
            bounds.setLeft(std::min(bounds.left(), pt.x()));
            bounds.setRight(std::max(bounds.right(), pt.x()));
            bounds.setTop(std::min(bounds.top(), pt.y()));
            bounds.setBottom(std::max(bounds.bottom(), pt.y()));
        }
        return bounds.adjusted(-step.size / 2.0 - 4, -step.size / 2.0 - 4,
                               step.size / 2.0 + 4, step.size / 2.0 + 4);
    }
    if (step.shape == MaskShape::Arrow || step.shape == MaskShape::Line
        || step.shape == MaskShape::Highlight) {
        QRectF box(step.line.p1(), step.line.p2());
        const qreal pad = std::max(6.0, step.size / 2.0 + 4.0);
        return box.normalized().adjusted(-pad, -pad, pad, pad);
    }
    return step.pixels;
}

int Masks::hitTest(const QPointF &point) const
{
    for (int i = int(count) - 1; i >= 0; --i) {
        const Step &step = steps.at(i);
        if (step.shape == MaskShape::Ellipse) {
            const QPointF center(step.pixels.center());
            const qreal rx = step.pixels.width() / 2.0, ry = step.pixels.height() / 2.0;
            if (rx <= 0 || ry <= 0) continue;
            const qreal nx = (point.x() - center.x()) / rx;
            const qreal ny = (point.y() - center.y()) / ry;
            if (nx * nx + ny * ny <= 1.0) return i;
        } else if (step.shape == MaskShape::Arrow || step.shape == MaskShape::Line
                   || step.shape == MaskShape::Highlight) {
            const QPointF delta = step.line.p2() - step.line.p1();
            const qreal length2 = delta.x() * delta.x() + delta.y() * delta.y();
            qreal t = 0;
            if (length2 > 0.001)
                t = ((point.x() - step.line.p1().x()) * delta.x()
                     + (point.y() - step.line.p1().y()) * delta.y()) / length2;
            t = std::clamp(t, 0.0, 1.0);
            const QPointF nearest = step.line.p1() + t * delta;
            if (QLineF(point, nearest).length() <= std::max(6.0, step.size / 2.0 + 5.0)) return i;
        } else if (step.shape == MaskShape::Text) {
            if (textBounds(step).contains(point)) return i;
        } else if (step.shape == MaskShape::Pen || step.shape == MaskShape::MosaicBrush) {
            const qreal tolerance = std::max(6.0, step.size / 2.0 + 4.0);
            for (int i2 = 0; i2 + 1 < step.points.size(); ++i2) {
                QLineF seg(step.points.at(i2), step.points.at(i2 + 1));
                QPointF between = seg.pointAt(std::clamp(QLineF(seg.p1(), point).length()
                                                     / std::max(seg.length(), 0.001), 0.0, 1.0));
                if (QLineF(point, between).length() <= tolerance) return i;
            }
        } else if (step.pixels.contains(point.toPoint())) {
            return i;
        }
    }
    return -1;
}

int Masks::maxNumberText() const
{
    int max = 0;
    for (qsizetype i = 0; i < count; ++i) {
        const Step &step = steps.at(i);
        if (step.shape != MaskShape::Number) continue;
        bool ok = false;
        const int value = step.text.toInt(&ok);
        if (ok && value > max) max = value;
    }
    return max;
}

bool Masks::moveStep(int index, const QPointF &delta)
{
    if (index < 0 || index >= count) return false;
    Step &step = steps[index];
    QPoint offset = delta.toPoint();
    if (step.shape == MaskShape::Text) {
        QPoint topLeft = step.pixels.topLeft() + offset;
        topLeft.setX(std::clamp(topLeft.x(), 0, std::max(0, original.width() - 1)));
        topLeft.setY(std::clamp(topLeft.y(), 0, std::max(0, original.height() - 1)));
        const QPoint shift = topLeft - step.pixels.topLeft();
        step.pixels = QRect(topLeft, original.rect().bottomRight());
        step.line.translate(shift);
    } else {
        if (step.pixels.left() + offset.x() < 0) offset.rx() = -step.pixels.left();
        if (step.pixels.top() + offset.y() < 0) offset.ry() = -step.pixels.top();
        if (step.pixels.right() + offset.x() >= original.width())
            offset.rx() = original.width() - 1 - step.pixels.right();
        if (step.pixels.bottom() + offset.y() >= original.height())
            offset.ry() = original.height() - 1 - step.pixels.bottom();
        step.pixels.translate(offset);
        step.line.translate(offset);
        step.points.translate(offset);
    }
    rebuild();
    return !offset.isNull();
}

void Masks::setStepColor(int index, const QColor &color)
{
    if (index < 0 || index >= count) return;
    QColor ink = color;
    if (steps.at(index).shape == MaskShape::Highlight) ink.setAlpha(140);
    else if (steps.at(index).shape == MaskShape::Number) ink.setAlpha(245);
    else ink.setAlpha(255);
    steps[index].color = ink;
    rebuild();
}

void Masks::removeStep(int index)
{
    if (index < 0 || index >= count) return;
    steps = steps.mid(0, count);
    steps.removeAt(index);
    count = steps.size();
    rebuild();
}

void Masks::undo()
{
    if (!canUndo()) return;
    --count;
    rebuild();
}

void Masks::redo()
{
    if (canRedo()) apply(steps.at(count++));
}
