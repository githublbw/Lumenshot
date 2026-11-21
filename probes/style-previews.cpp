// 离屏渲染箭头与序号的候选样式，供用户挑选后落实到 src/masks.cpp。
// 构建：g++ probes/style-previews.cpp -o /tmp/style-previews $(Qt6 Gui/QtCore)
// 运行：QT_QPA_PLATFORM=offscreen /tmp/style-previews 输出目录
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRadialGradient>
#include <QFont>
#include <QLineF>
#include <QPointF>
#include <QDir>
#include <cmath>
#include <cstdio>

static QPointF perp(const QPointF &direction) { return {-direction.y(), direction.x()}; }

// 每套样式都接收同一条线段与线宽，保证横向可比。
static void drawArrow(QPainter &p, const QLineF &line, const QColor &color, qreal width, int style)
{
    const QPointF delta = line.p2() - line.p1();
    const qreal length = std::hypot(delta.x(), delta.y());
    if (length < 1) return;
    const QPointF direction = delta / length;
    const QPointF wing = perp(direction);
    p.setRenderHint(QPainter::Antialiasing);
    if (style == 1) { // A 经典三角头：细杆 + 纯三角头
        const qreal head = std::clamp(length * 0.30, width * 5.0, length * 0.45);
        const qreal half = head * 0.34;
        const QPointF base = line.p2() - direction * head;
        p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(line.p1(), base);
        QPainterPath headPath;
        headPath.moveTo(line.p2());
        headPath.lineTo(base + wing * half);
        headPath.lineTo(base - direction * head * 0.06);
        headPath.lineTo(base - wing * half);
        headPath.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(headPath);
    } else if (style == 2) { // B 开口燕尾：细杆 + 两笔张开的箭须
        const qreal head = std::clamp(length * 0.20, width * 4.5, length * 0.35);
        const QPointF tip = line.p2() - direction * width * 0.4;
        p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(line.p1(), tip - direction * head * 0.45);
        p.drawLine(tip, tip - direction * head + wing * head * 0.46);
        p.drawLine(tip, tip - direction * head - wing * head * 0.46);
    } else if (style == 3) { // C 锥形飞镖：尾细头粗的渐变杆 + 尖头
        const qreal head = std::clamp(length * 0.28, width * 5.0, length * 0.42);
        const qreal half = head * 0.34;
        const QPointF base = line.p2() - direction * head;
        QPainterPath body;
        body.moveTo(line.p1() + wing * (width * 0.38));
        body.lineTo(base + wing * (width * 0.62));
        body.lineTo(base - wing * (width * 0.62));
        body.lineTo(line.p1() - wing * (width * 0.38));
        body.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(body);
        QPainterPath headPath;
        headPath.moveTo(line.p2());
        headPath.lineTo(base + wing * half);
        headPath.lineTo(base - direction * head * 0.12);
        headPath.lineTo(base - wing * half);
        headPath.closeSubpath();
        p.drawPath(headPath);
    } else if (style == 4) { // D 起点圆点：圆点起点 + 中等尖头
        const qreal head = std::clamp(length * 0.24, width * 4.5, length * 0.38);
        const qreal half = head * 0.36;
        const QPointF base = line.p2() - direction * head;
        p.setPen(QPen(color, width * 0.85, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(line.p1() + direction * width * 1.6, base);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(line.p1(), width * 1.6, width * 1.6);
        QPainterPath headPath;
        headPath.moveTo(line.p2());
        headPath.lineTo(base + wing * half);
        headPath.lineTo(base - direction * head * 0.08);
        headPath.lineTo(base - wing * half);
        headPath.closeSubpath();
        p.drawPath(headPath);
    } else { // E 极简细线：更细的杆 + 小巧紧致的头
        const qreal head = std::clamp(length * 0.17, width * 3.5, length * 0.28);
        const qreal half = head * 0.40;
        const QPointF base = line.p2() - direction * head;
        p.setPen(QPen(color, width * 0.6, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(line.p1(), base);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(QPolygonF() << line.p2() << base + wing * half << base - wing * half);
    }
}

static void drawBadge(QPainter &p, const QRectF &box, int style, const QColor &base, const QString &text)
{
    p.setRenderHint(QPainter::Antialiasing);
    const qreal size = std::min(box.width(), box.height());
    const QPointF center = box.center();
    QFont font;
    font.setBold(true);
    font.setPixelSize(qMax(8, qRound(size * 0.56)));
    QColor ink = qGray(base.rgb()) > 168 ? QColor("#101014") : QColor("#FFFFFF");
    p.setFont(font);
    if (style == 1) { // 1 扁平纯色圆：无渐变、无描边
        p.setPen(Qt::NoPen);
        p.setBrush(base);
        p.drawEllipse(center, size / 2, size / 2);
        p.setPen(ink);
        p.drawText(box, Qt::AlignCenter, text);
    } else if (style == 2) { // 2 深底彩环：深色圆底 + 彩色描边
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(21, 23, 27, 235));
        p.drawEllipse(center, size / 2, size / 2);
        p.setPen(QPen(base, std::max(1.5, size * 0.09)));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, size / 2 - p.pen().widthF() / 2, size / 2 - p.pen().widthF() / 2);
        p.setPen(ink);
        p.drawText(box, Qt::AlignCenter, text);
    } else if (style == 3) { // 3 圆角方形标签
        const QRectF tile = box.adjusted(size * 0.08, size * 0.08, -size * 0.08, -size * 0.08);
        p.setPen(Qt::NoPen);
        p.setBrush(base);
        p.drawRoundedRect(tile, tile.width() * 0.26, tile.width() * 0.26);
        p.setPen(ink);
        p.drawText(tile, Qt::AlignCenter, text);
    } else if (style == 4) { // 4 胶囊标签：横向加长的药丸形
        const qreal height = size * 0.72;
        const qreal width = size * 1.55;
        const QRectF pill(center - QPointF(width / 2, height / 2), QSizeF(width, height));
        p.setPen(Qt::NoPen);
        p.setBrush(base);
        p.drawRoundedRect(pill, height / 2, height / 2);
        p.setPen(ink);
        p.drawText(pill, Qt::AlignCenter, text);
    } else { // 5 无底色描边数字：白描边 + 彩色数字，任何背景都清晰
        QPainterPath glyph;
        glyph.addText(center - QPointF(0, size * 0.02), font, text);
        glyph.translate(center - glyph.boundingRect().center());
        p.setPen(QPen(QColor(255, 255, 255, 235), size * 0.14, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(glyph);
        p.setPen(Qt::NoPen);
        p.setBrush(base);
        p.drawPath(glyph);
    }
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const QString out = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("mockups/style-picks");
    QDir().mkpath(out);
    const QColor canvas(32, 35, 41);       // 接近深色桌面的背景
    const QColor ink(255, 95, 87);         // 箭头统一用红色示例
    const QColor badge(10, 132, 255);      // 序号统一用蓝色示例

    for (int style = 1; style <= 5; ++style) {
        QImage image(480, 240, QImage::Format_ARGB32);
        image.fill(canvas);
        QPainter p(&image);
        drawArrow(p, QLineF(60, 80, 320, 60), ink, 4, style);
        drawArrow(p, QLineF(90, 185, 190, 175), ink, 4, style);
        p.setPen(QColor("#8E8E97"));
        QFont font;
        font.setPixelSize(13);
        p.setFont(font);
        p.drawText(QRect(330, 40, 140, 120), Qt::AlignLeft,
                   QStringLiteral("长 %1px\n短 %2px\n杆宽 %3px").arg(260).arg(100).arg(4));
        p.end();
        image.save(QString("%1/arrow-%2.png").arg(out).arg(style));
    }

    for (int style = 1; style <= 5; ++style) {
        QImage image(560, 200, QImage::Format_ARGB32);
        image.fill(canvas);
        QPainter p(&image);
        drawBadge(p, QRectF(40, 80, 28, 28), style, badge, QStringLiteral("1"));
        drawBadge(p, QRectF(200, 40, 120, 120), style, badge, QStringLiteral("1"));
        p.setPen(QColor("#8E8E97"));
        QFont font;
        font.setPixelSize(13);
        p.setFont(font);
        p.drawText(QRect(380, 60, 160, 100), Qt::AlignLeft,
                   QStringLiteral("实际 24px\n右侧为 4 倍放大"));
        p.end();
        image.save(QString("%1/number-%2.png").arg(out).arg(style));
    }

    { // 汇总对比：箭头五行
        QImage image(560, 5 * 130 + 30, QImage::Format_ARGB32);
        image.fill(canvas);
        QPainter p(&image);
        QFont font;
        font.setPixelSize(15);
        font.setBold(true);
        p.setFont(font);
        const char *names = "ABCDE";
        for (int style = 1; style <= 5; ++style) {
            const int top = 30 + (style - 1) * 130;
            p.setPen(QColor("#A3F0CE"));
            p.drawText(QRect(14, top + 40, 40, 40), Qt::AlignCenter, QString(names[style - 1]));
            drawArrow(p, QLineF(70, top + 62, 330, top + 52), ink, 4, style);
            drawArrow(p, QLineF(370, top + 66, 460, top + 60), ink, 4, style);
        }
        p.end();
        image.save(QString("%1/arrows-all.png").arg(out));
    }
    { // 汇总对比：序号五行
        QImage image(560, 5 * 130 + 30, QImage::Format_ARGB32);
        image.fill(canvas);
        QPainter p(&image);
        QFont font;
        font.setPixelSize(15);
        font.setBold(true);
        p.setFont(font);
        for (int style = 1; style <= 5; ++style) {
            const int top = 30 + (style - 1) * 130;
            p.setPen(QColor("#A3F0CE"));
            p.drawText(QRect(14, top + 45, 40, 40), Qt::AlignCenter, QString::number(style));
            drawBadge(p, QRectF(80, top + 48, 28, 28), style, badge, QStringLiteral("1"));
            drawBadge(p, QRectF(190, top + 22, 84, 84), style, badge, QStringLiteral("1"));
            drawBadge(p, QRectF(330, top + 40, 72, 72), style, badge, QStringLiteral("12"));
        }
        p.end();
        image.save(QString("%1/numbers-all.png").arg(out));
    }
    std::printf("saved to %s\n", qPrintable(out));
    return 0;
}
