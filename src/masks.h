#pragma once

#include <QImage>
#include <QList>
#include <QLineF>
#include <QPolygonF>

enum class MaskShape { Rectangle, Ellipse, RoundedRect, Bubble, Arrow, Line, Highlight,
                       Number, Text, Blur, Mosaic, MosaicBrush, Pen, Crop };

class Masks {
public:
    void reset(QImage image);
    void add(MaskShape shape, QRectF bounds, QColor color, int size = 12, QString text = {},
             int alphaOverride = -1);
    // 自由笔迹：画笔（描边）与马赛克笔刷（沿路径像素化）
    void add(MaskShape shape, const QPolygonF &points, QColor color, int size);
    void crop(const QRect &area);
    int stepCount() const { return int(count); }
    MaskShape stepShape(int index) const;
    // 拖动预览：把指定标注按偏移绘制到外部画布（模糊/马赛克绘制占位块）
    void paintStep(QPainter &painter, int index, const QPointF &offset) const;
    QRectF stepBounds(int index) const;
    int hitTest(const QPointF &point) const;
    int maxNumberText() const;
    bool moveStep(int index, const QPointF &delta);
    void setStepColor(int index, const QColor &color);
    void setStepSize(int index, int size);        // 线宽/字号/笔刷块大小
    void setStepAlpha(int index, int percent);    // 仅作用于填充类形状
    void setStepTextWidth(int index, int width);  // 文本换行宽度
    void removeStep(int index);
    void undo();
    void redo();
    bool canUndo() const { return count > 0; }
    bool canRedo() const { return count < steps.size(); }
    bool isLarge() const { return qint64(output.width()) * output.height() >= 1000000; }
    const QImage &image() const { return output; }

private:
    struct Step {
        MaskShape shape; QRect pixels; QColor color; QLineF line;
        int size; QString text; QPolygonF points; int wrap = 0;
    };
    void apply(const Step &step);
    void rebuild();
    QRectF textBounds(const Step &step) const;
    QImage original, output;
    QList<Step> steps;
    qsizetype count = 0;
};
