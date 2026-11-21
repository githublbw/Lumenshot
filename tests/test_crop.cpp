#include "crop.h"

#include <iostream>

int main()
{
    QImage image(12, 8, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            image.setPixel(x, y, qRgba(x, y, 10, 180));

    struct Case {
        const char *name;
        QRectF bounds;
        QPointF start, end;
        QRect expected;
    };
    const QRectF normal(0, 0, 12, 8), secondary(-12, -4, 12, 8), scaled(0, 0, 6, 4);
    const Case checks[] = {
        {"forward", normal, {2, 1}, {5, 4}, {2, 1, 3, 3}},
        {"reverse", normal, {5, 4}, {2, 1}, {2, 1, 3, 3}},
        {"negative origin", secondary, {-10, -3}, {-7, 0}, {2, 1, 3, 3}},
        {"clip both ends", normal, {-2, 3}, {15, 9}, {0, 3, 12, 5}},
        {"last pixel", normal, {11, 7}, {12, 8}, {11, 7, 1, 1}},
        {"2x", scaled, {1, 1}, {3, 2}, {2, 2, 4, 2}},
        {"fractional scaling", {0, 0, 8, 8}, {1, 1}, {3, 3}, {1, 1, 4, 2}},
        {"outside", normal, {13, 3}, {20, 6}, {}},
        {"zero width", normal, {2, 2}, {2, 6}, {}},
        {"zero height", normal, {1, 3}, {5, 3}, {}},
        {"empty bounds", {}, {0, 0}, {5, 3}, {}},
    };
    for (const auto &c : checks) {
        const auto cropped = lumenshot::cropSelection(image, c.bounds, c.start, c.end);
        const auto expected = c.expected.isEmpty() ? QImage{} : image.copy(c.expected);
        if (cropped != expected) {
            std::cerr << c.name << " failed\n";
            return 1;
        }
    }
    return 0;
}
