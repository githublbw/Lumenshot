#pragma once

#include <QImage>
#include <QRectF>

namespace lumenshot {
// Bounds and drag endpoints share one logical coordinate space for this image.
QImage cropSelection(const QImage &image, const QRectF &bounds, QPointF start, QPointF end);
}
