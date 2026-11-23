#include "masks.h"
#include <QBuffer>
#include <QPainter>
#include <QGuiApplication>
#include <iostream>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QImage original(20, 14, QImage::Format_ARGB32);
    for (int y = 0; y < original.height(); ++y)
        for (int x = 0; x < original.width(); ++x)
            original.setPixel(x, y, qRgba(x, y, 10, 70));
    Masks model;
    model.reset(original);
    QColor color(150, 80, 35, 8);
    model.add(MaskShape::Rectangle, QRectF(8, 8, -12, -6), color);
    // 空心矩形：内部保持原图像素，边框为标注色
    const QImage expected = model.image();
    if (expected.pixel(4, 4) != original.pixel(4, 4) || expected == original) return 1;
    if (expected.pixel(0, 4) != color.rgb() || expected.pixel(7, 5) != color.rgb()) return 1;
    if (!model.canUndo()) return 1;
    model.add(MaskShape::Ellipse, {4, 3, 12, 8}, Qt::black);
    const QImage ellipse = model.image();
    // 空心椭圆：内部不填充，只有轮廓被描边
    if (ellipse.pixel(10, 7) != expected.pixel(10, 7)) return 2;
    if (ellipse == expected) return 2;
    if (ellipse.pixel(4, 7) != qRgb(0,0,0)) return 2;
    // 描边以轮廓线为中心，允许超出外接矩形约半个线宽（含抗锯齿），更远处必须保持原样
    const QRect painted = QRect(4, 3, 12, 8).adjusted(-2, -2, 2, 2);
    for (int y = 0; y < ellipse.height(); ++y)
        for (int x = 0; x < ellipse.width(); ++x)
            if (!painted.contains(x,y) && ellipse.pixel(x,y) != expected.pixel(x,y))
                return 3;
    model.undo();
    if (model.image() != expected) return 4;
    model.undo();
    if (model.image() != original || model.canUndo()) return 5;
    model.redo();
    model.redo();
    if (ellipse != model.image() || model.canRedo()) return 6;
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    if (!model.image().save(&buffer, "PNG") ||
        QImage::fromData(buffer.data()).convertToFormat(original.format()) != ellipse) return 7;
    model.undo();
    model.add(MaskShape::Rectangle, {21, 1, 3, 2}, color);
    model.add(MaskShape::Rectangle, {1, 1, 0, 3}, color);
    if (!model.canRedo() || expected != model.image()) return 8;
    model.add(MaskShape::Ellipse, {1,1,4,4}, Qt::white);
    if (model.canRedo()) return 9;
    model.reset(original);
    if (model.canUndo() || model.canRedo() || model.image() != original) return 10;
    for (auto shape : {MaskShape::Blur, MaskShape::Mosaic}) {
        model.add(shape, {18,12,-16,-8}, {}, 5);
        if (model.image() == original) return 11;
        for (int y = 0; y < original.height(); ++y)
            for (int x = 0; x < original.width(); ++x)
                if (!QRect(2,4,16,8).contains(x,y) && model.image().pixel(x,y) != original.pixel(x,y))
                    return 12;
        model.undo();
        if (model.image() != original) return 13;
    }
    model.add(MaskShape::Arrow, {17,7,-15,0}, Qt::red, 2);
    if (model.image() == original) return 14;
    model.undo();
    model.add(MaskShape::Text,{1,0,0,0}, Qt::black, 10, " ");
    if (model.canUndo()) return 15;
    model.add(MaskShape::Text,{1,0,0,0}, Qt::black, 10, QStringLiteral("图"));
    if (model.image() == original || model.canRedo()) return 16;

    Masks edit;
    QImage canvas(40, 30, QImage::Format_ARGB32);
    canvas.fill(qRgb(10, 12, 14));
    edit.reset(canvas);
    const QColor ink(255, 120, 20);
    edit.add(MaskShape::Rectangle, {5, 5, 10, 8}, ink);
    if (edit.hitTest(QPointF(8, 8)) != 0 || edit.hitTest(QPointF(30, 20)) != -1) return 17;
    const QPointF before = edit.stepBounds(0).topLeft();
    edit.moveStep(0, QPointF(6, 4));
    if (edit.stepBounds(0).topLeft() != before + QPointF(6, 4)) return 18;
    edit.moveStep(0, QPointF(-100, -100));
    if (edit.stepBounds(0).topLeft() != QPointF(0, 0)) return 19;
    edit.setStepColor(0, QColor(0, 160, 255));
    // 空心矩形换色后：边框更新为新颜色，内部仍是画布原样
    if (edit.image().pixel(0, 2) != qRgb(0, 160, 255)) return 20;
    if (edit.image().pixel(5, 4) != canvas.pixel(5, 4)) return 20;
    edit.removeStep(0);
    if (edit.stepCount() != 0 || edit.image() != canvas) return 21;

    edit.reset(canvas);
    edit.add(MaskShape::Rectangle, {10, 10, 6, 6}, ink);
    edit.crop(QRect(8, 8, 20, 16));
    if (edit.image().size() != QSize(20, 16) || edit.stepCount() != 1) return 22;
    if (!edit.stepBounds(0).contains(QPointF(3, 3)) || edit.hitTest(QPointF(4, 4)) != 0)
        return 23;

    edit.reset(canvas);
    edit.add(MaskShape::Number, {5, 5, 0, 0}, Qt::black, 20, "1");
    edit.add(MaskShape::Number, {12, 8, 0, 0}, Qt::black, 20, "2");
    if (edit.maxNumberText() != 2) return 24;
    edit.undo();
    if (edit.maxNumberText() != 1) return 25;
    edit.removeStep(0);
    if (edit.maxNumberText() != 0) return 26;

    // 画笔：折线描边像素化 + 可选中/移动
    edit.reset(canvas);
    QPolygonF stroke;
    stroke << QPointF(6, 6) << QPointF(16, 10) << QPointF(10, 18);
    edit.add(MaskShape::Pen, stroke, QColor(255, 0, 0), 3);
    if (edit.stepCount() != 1 || edit.image() == canvas) return 27;
    if (edit.hitTest(QPointF(11, 10)) != 0) return 28;
    edit.setStepSize(0, 6);
    if (edit.stepBounds(0).isEmpty()) return 29;
    const QImage penBefore = edit.image();
    edit.moveStep(0, QPointF(4, 0));
    if (edit.image() == penBefore) return 30;
    edit.removeStep(0);
    if (edit.image() != canvas) return 31;

    // 圆角框：空心描边，内部透出画布
    edit.add(MaskShape::RoundedRect, QRectF(4, 4, 14, 12), QColor(0, 120, 255, 90), 12, QString(), 60);
    if (edit.stepCount() != 1) return 32;
    if (edit.image() == canvas) return 33;
    if (edit.image().pixel(10, 10) != canvas.pixel(10, 10)) return 33;
    edit.setStepAlpha(0, 100);
    if (edit.image().pixel(4, 10) != qRgb(0, 120, 255)) return 34;

    // 对话气泡：拖动起点在气泡角上，尾巴沿对角向外伸出
    edit.reset(canvas);
    edit.add(MaskShape::Bubble, QRectF(QPointF(12, 22), QPointF(30, 10)), QColor(255, 160, 0),
             12, QString(), -1);
    if (edit.stepCount() != 1 || edit.image() == canvas) return 35;
    if (edit.image().pixel(20, 16) != qRgb(255, 160, 0)) return 36;
    if (edit.image().pixel(5, 25) == canvas.pixel(5, 25)) return 37;

    // 马赛克笔刷：沿路径像素化，涂抹处与原内容不同
    edit.reset(canvas);
    QPolygonF underline;
    underline << QPointF(4, 5) << QPointF(32, 5);
    edit.add(MaskShape::Pen, underline, QColor(255, 255, 255), 6);
    const QImage withLine = edit.image();
    QPolygonF smear;
    smear << QPointF(8, 5) << QPointF(20, 7);
    edit.add(MaskShape::MosaicBrush, smear, QColor(), 10);
    if (edit.stepCount() != 2) return 38;
    if (edit.image() == withLine) return 39;
    edit.setStepSize(1, 16);
    if (edit.stepBounds(1).isEmpty()) return 40;

    // 文本换行宽度调整（宽画布：默认换行 420，收到 80）
    QImage wide(480, 40, QImage::Format_RGB32);
    wide.fill(qRgb(10, 12, 14));
    edit.reset(wide);
    edit.add(MaskShape::Text, {2, 2, 0, 0}, QColor("#FFFFFF"), 10, QStringLiteral("宽度测试文本宽度测试文本宽度测试"));
    const int originalWidth = int(edit.stepBounds(0).width());
    edit.setStepTextWidth(0, 80);
    if (edit.stepBounds(0).width() >= originalWidth) return 41;
    if (edit.stepBounds(0).width() < 60) return 42;
    // 移动文字后换行宽度保持不变、内容移动
    edit.moveStep(0, QPointF(30, 10));
    const QImage moved = edit.image();
    if (moved == wide) return 43;
    edit.setStepTextWidth(0, 80);
    if (edit.stepBounds(0).width() < 60 || edit.stepBounds(0).width() > 80) return 44;
    // paintStep 拖动预览：内容按偏移绘制到外部画布
    {
        QImage base = edit.image();
        QImage before = base.copy();
        QPainter pv(&base);
        edit.paintStep(pv, 0, QPointF(60, 10));
        pv.end();
        if (base == before) return 45;
        // 偏移处应出现文字像素，而原位置仍是提交时的样子
        const QRectF nb = edit.stepBounds(0).translated(60, 10);
        QImage probe = base.copy(nb.toAlignedRect().adjusted(-4, -4, 4, 4));
        if (probe == before.copy(nb.toAlignedRect().adjusted(-4, -4, 4, 4))) return 46;
    }
    std::cout << "Annotation pixels, history and PNG round trip passed\n";
}
