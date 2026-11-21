#include "crop.h"

#include <cmath>

namespace lumenshot {
QImage cropSelection(const QImage &image, const QRectF &bounds, QPointF start, QPointF end)
{
    if (image.isNull() || !bounds.isValid())
        return {};

    const QRectF selection = QRectF(start, end).normalized().intersected(bounds);
    if (selection.isEmpty())
        return {};

    // Include partially covered pixels and keep the right/bottom endpoints exclusive.
    const auto x = [&](double v) { return (v - bounds.x()) * image.width() / bounds.width(); };
    const auto y = [&](double v) { return (v - bounds.y()) * image.height() / bounds.height(); };
    const int left = std::floor(x(selection.left()));
    const int top = std::floor(y(selection.top()));
    const int right = std::ceil(x(selection.right()));
    const int bottom = std::ceil(y(selection.bottom()));
    const QRect pixels = QRect(left, top, right - left, bottom - top).intersected(image.rect());
    return pixels.isEmpty() ? QImage{} : image.copy(pixels);
}
}
