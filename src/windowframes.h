#pragma once

#include <QList>
#include <QRect>
#include <QImage>
class QScreen;

struct WindowFrame {
    QRect bounds;
    bool selectable;
};

// Visible root children in front-to-back order, in X11 root pixels.
QList<WindowFrame> captureWindowFrames(QScreen *screen, QImage &image);
