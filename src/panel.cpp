#include "hotkey.h"
#include "preferences.h"
#include "masks.h"
#include "windowframes.h"
#include <QtWidgets>
#include <QWindow>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QElapsedTimer>
#include <QSettings>
#include <QBuffer>
#include <QImageReader>
#include <algorithm>
#include <cmath>
#include <QPointer>
#include <QtConcurrentRun>
#ifdef LUMENSHOT_OCR
#include "ocr.h"
#endif

namespace {
const QColor kMint("#A3F0CE");
const QColor kIcon("#D2D2D8");
const QColor kMuted("#8E8E97");

QIcon icon(const QString &name)
{
    return QIcon(":/" + name + ".png");
}

QPixmap tinted(const QString &name, const QColor &color, int size)
{
    constexpr qreal dpr = 2.0;
    const QPixmap source(":/" + name + ".png");
    QPixmap out(qRound(size * dpr), qRound(size * dpr));
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPixmap scaled = source.scaled(qRound(size * dpr), qRound(size * dpr),
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    painter.drawPixmap(QPointF(0, 0), scaled);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(QRectF(0, 0, size, size), color);
    return out;
}

QIcon iconTint(const QString &name, const QColor &color, int size = 17)
{
    return QIcon(tinted(name, color, size));
}

// 统一品牌图标：圆形光晕底 + 截图框图形，标题栏 / 托盘 / 任务栏 / 启动器共用。
QPixmap brandOrb(int size)
{
    constexpr qreal dpr = 2.0;
    QPixmap out(qRound(size * dpr), qRound(size * dpr));
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRectF circle(1.0, 1.0, size - 2.0, size - 2.0);

    QRadialGradient glow(circle.center(), circle.width() / 2);
    glow.setColorAt(0.0, QColor(163, 240, 206, 38));
    glow.setColorAt(0.62, QColor(163, 240, 206, 10));
    glow.setColorAt(1.0, QColor(163, 240, 206, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 10, 12, 235));
    painter.drawEllipse(circle);
    painter.setBrush(glow);
    painter.drawEllipse(circle);

    painter.setPen(QPen(QColor(163, 240, 206, 110), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(circle.adjusted(0.5, 0.5, -0.5, -0.5));

    const int glyph = qRound(size * 0.44);
    painter.drawPixmap(QPointF((size - glyph) / 2.0, (size - glyph) / 2.0),
                       tinted("focus", kMint, glyph));
    return out;
}

QIcon brandIcon()
{
    QIcon out;
    for (int s : {16, 22, 24, 32, 48, 64}) out.addPixmap(brandOrb(s));
    return out;
}

QPixmap controlGlyph(const QString &kind, const QColor &color, int size = 14)
{
    constexpr qreal dpr = 2.0;
    QPixmap out(size * dpr, size * dpr);
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal m = 3.2;
    if (kind == "minimize") {
        painter.drawLine(QPointF(m, size / 2.0), QPointF(size - m, size / 2.0));
    } else if (kind == "maximize") {
        painter.drawRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), 2, 2);
    } else if (kind == "restore") {
        const qreal o = 1.8;
        painter.drawRoundedRect(QRectF(m, m + o, size - 2 * m - o, size - 2 * m - o), 2, 2);
        painter.drawRoundedRect(QRectF(m + o, m, size - 2 * m - o, size - 2 * m - o), 2, 2);
    } else {
        painter.drawLine(QPointF(m, m), QPointF(size - m, size - m));
        painter.drawLine(QPointF(size - m, m), QPointF(m, size - m));
    }
    return out;
}

// 新工具没有现成资源图，用少量线条按同一视觉规格现场绘制。
QPixmap shapeGlyph(const QString &kind, const QColor &color, int size = 17)
{
    constexpr qreal dpr = 2.0;
    QPixmap out(qRound(size * dpr), qRound(size * dpr));
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const qreal m = size * 0.22;
    if (kind == "line") {
        p.drawLine(QPointF(m, size - m), QPointF(size - m, m));
    } else if (kind == "highlighter") {
        QColor ink = color;
        ink.setAlpha(190);
        p.setPen(QPen(ink, size * 0.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(m, size - m * 0.7), QPointF(size - m * 0.7, m));
        p.setPen(QPen(color, 1.2));
        p.drawLine(QPointF(size - m, m * 0.8), QPointF(size - m * 0.62, m * 0.62));
    } else if (kind == "number") {
        p.drawEllipse(QRectF(m * 0.75, m * 0.75, size - 1.5 * m, size - 1.5 * m));
        QFont font = p.font();
        font.setPixelSize(qMax(7, qRound(size * 0.48)));
        font.setBold(true);
        p.setFont(font);
        p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, "1");
    } else if (kind == "clock") {
        const QRectF face(m, m, size - 2 * m, size - 2 * m);
        p.drawEllipse(face);
        p.drawLine(face.center(), face.center() + QPointF(0, -face.height() * 0.28));
        p.drawLine(face.center(), face.center() + QPointF(face.width() * 0.22, 0));
    } else if (kind == "history") {
        const QRectF face(m, m, size - 2 * m, size - 2 * m);
        p.drawArc(face, 40 * 16, 280 * 16);
        const QPointF tip(face.center().x() + face.width() * 0.42,
                          face.center().y() - face.height() * 0.28);
        p.drawLine(tip, tip + QPointF(-face.width() * 0.20, -face.height() * 0.04));
        p.drawLine(tip, tip + QPointF(-face.width() * 0.03, face.height() * 0.20));
    } else if (kind == "pointer") {
        QPainterPath arrow;
        arrow.moveTo(size * 0.30, size * 0.18);
        arrow.lineTo(size * 0.30, size * 0.80);
        arrow.lineTo(size * 0.45, size * 0.64);
        arrow.lineTo(size * 0.56, size * 0.87);
        arrow.lineTo(size * 0.67, size * 0.81);
        arrow.lineTo(size * 0.56, size * 0.59);
        arrow.lineTo(size * 0.76, size * 0.56);
        arrow.closeSubpath();
        p.setBrush(color);
        p.drawPath(arrow);
    } else if (kind == "pen") {
        // 铅笔：斜杆 + 笔尖三角
        p.drawLine(QPointF(size * 0.30, size * 0.74), QPointF(size * 0.72, size * 0.28));
        QPainterPath tip;
        tip.moveTo(size * 0.26, size * 0.78);
        tip.lineTo(size * 0.36, size * 0.80);
        tip.lineTo(size * 0.28, size * 0.70);
        tip.closeSubpath();
        p.setBrush(color);
        p.drawPath(tip);
    } else if (kind == "rounded") {
        p.drawRoundedRect(QRectF(size * 0.18, size * 0.30, size * 0.64, size * 0.42), 5, 5);
    } else if (kind == "bubble") {
        QPainterPath bubbleGlyph;
        bubbleGlyph.addRoundedRect(QRectF(size * 0.16, size * 0.24, size * 0.68, size * 0.44), 6, 6);
        bubbleGlyph.moveTo(size * 0.30, size * 0.70);
        bubbleGlyph.lineTo(size * 0.22, size * 0.84);
        bubbleGlyph.lineTo(size * 0.42, size * 0.70);
        p.drawPath(bubbleGlyph);
    } else if (kind == "mbrush") {
        // 马赛克笔刷：错落的方块
        p.setBrush(color);
        p.drawRect(QRectF(size * 0.20, size * 0.24, 6, 6));
        p.drawRect(QRectF(size * 0.38, size * 0.40, 6, 6));
        p.drawRect(QRectF(size * 0.56, size * 0.30, 6, 6));
        p.drawRect(QRectF(size * 0.44, size * 0.58, 6, 6));
        p.drawRect(QRectF(size * 0.62, size * 0.66, 6, 6));
    } else if (kind == "sliders") {
        // 粗细/透明度调节
        p.drawLine(QPointF(size * 0.18, size * 0.34), QPointF(size * 0.82, size * 0.34));
        p.setBrush(color);
        p.drawEllipse(QPointF(size * 0.58, size * 0.34), 2.6, 2.6);
        p.drawLine(QPointF(size * 0.18, size * 0.66), QPointF(size * 0.82, size * 0.66));
        p.drawEllipse(QPointF(size * 0.34, size * 0.66), 2.6, 2.6);
    } else if (kind == "plus") {
        const qreal center = size * 0.5;
        p.drawLine(QPointF(center, m), QPointF(center, size - m));
        p.drawLine(QPointF(m, center), QPointF(size - m, center));
    } else if (kind == "scan-text") {
        // 四角取景框 + 三行文本，表示“识别截图中的文字”。
        const qreal inset = size * 0.18, arm = size * 0.22;
        const QPointF tl(inset, inset), tr(size - inset, inset),
                      bl(inset, size - inset), br(size - inset, size - inset);
        p.drawLine(tl, tl + QPointF(arm, 0));   p.drawLine(tl, tl + QPointF(0, arm));
        p.drawLine(tr, tr - QPointF(arm, 0));   p.drawLine(tr, tr + QPointF(0, arm));
        p.drawLine(bl, bl + QPointF(arm, 0));   p.drawLine(bl, bl - QPointF(0, arm));
        p.drawLine(br, br - QPointF(arm, 0));   p.drawLine(br, br - QPointF(0, arm));
        p.drawLine(QPointF(size * 0.30, size * 0.40), QPointF(size * 0.70, size * 0.40));
        p.drawLine(QPointF(size * 0.30, size * 0.52), QPointF(size * 0.62, size * 0.52));
        p.drawLine(QPointF(size * 0.30, size * 0.64), QPointF(size * 0.68, size * 0.64));
    } else if (kind == "crop") {
        p.drawLine(QPointF(m, m * 0.6), QPointF(m, size - m));
        p.drawLine(QPointF(m, size - m), QPointF(size - m * 0.6, size - m));
        p.drawLine(QPointF(m * 0.6, m), QPointF(size - m, m));
        p.drawLine(QPointF(size - m, m), QPointF(size - m, size - m * 0.6));
    } else if (kind == "clipboard") {
        p.drawRoundedRect(QRectF(m, m * 0.9, size - 2 * m, size - 1.9 * m), 2.5, 2.5);
        p.drawRoundedRect(QRectF(size / 2.0 - m * 0.45, m * 0.55, m * 0.9, m * 0.8), 1.5, 1.5);
        p.drawLine(QPointF(size * 0.34, size * 0.52), QPointF(size * 0.66, size * 0.52));
    } else if (kind == "sparkles") {
        const QPointF center(size * 0.45, size * 0.52);
        const qreal r = size * 0.32;
        QPainterPath star;
        star.moveTo(center.x(), center.y() - r);
        star.quadTo(center.x() + r * 0.16, center.y() - r * 0.16, center.x() + r, center.y());
        star.quadTo(center.x() + r * 0.16, center.y() + r * 0.16, center.x(), center.y() + r);
        star.quadTo(center.x() - r * 0.16, center.y() + r * 0.16, center.x() - r, center.y());
        star.quadTo(center.x() - r * 0.16, center.y() - r * 0.16, center.x(), center.y() - r);
        p.drawPath(star);
        p.drawLine(QPointF(size * 0.82, size * 0.22), QPointF(size * 0.82, size * 0.40));
        p.drawLine(QPointF(size * 0.73, size * 0.31), QPointF(size * 0.91, size * 0.31));
    } else if (kind == "image") {
        p.drawRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), 2.5, 2.5);
        p.drawEllipse(QPointF(size * 0.38, size * 0.40), size * 0.06, size * 0.06);
        p.drawPolyline(QPolygonF{QPointF(size * 0.28, size * 0.72), QPointF(size * 0.45, size * 0.54),
                                 QPointF(size * 0.58, size * 0.66), QPointF(size * 0.68, size * 0.57),
                                 QPointF(size * 0.78, size * 0.72)});
    }
    return out;
}

QIcon shapeIcon(const QString &kind, const QColor &color, int size = 17)
{
    return QIcon(shapeGlyph(kind, color, size));
}

QPixmap swatchIcon(const QColor &color, int size = 20)
{
    constexpr qreal dpr = 2.0;
    QPixmap out(size * dpr, size * dpr);
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF circle(1.5, 1.5, size - 3, size - 3);
    painter.setPen(QPen(QColor(0, 0, 0, 90), 1));
    painter.setBrush(color);
    painter.drawEllipse(circle);
    painter.setPen(QPen(QColor(255, 255, 255, 115), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(circle);
    return out;
}

QPixmap colorDot(const QColor &color, int size = 18, bool selected = false)
{
    constexpr qreal dpr = 2.0;
    QPixmap out(size * dpr, size * dpr);
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal inset = selected ? 3.0 : 1.5;
    painter.setPen(QPen(QColor(255, 255, 255, 70), 1));
    painter.setBrush(color);
    painter.drawEllipse(QRectF(inset, inset, size - inset * 2, size - inset * 2));
    if (selected) {
        painter.setPen(QPen(kMint, 1.4));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(1.2, 1.2, size - 2.4, size - 2.4));
    }
    return out;
}

// 浮层卡片统一加一层柔和投影，避免和深色背景糊在一起。
void addCardShadow(QWidget *widget, int blur = 22, int dy = 4, int alpha = 105)
{
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blur);
    shadow->setOffset(0, dy);
    shadow->setColor(QColor(0, 0, 0, alpha));
    widget->setGraphicsEffect(shadow);
}

// QSS 只能给出静态的 :hover 颜色，这里用一小段动画把背景淡入淡出。
class HoverButton : public QPushButton {
public:
    explicit HoverButton(QWidget *parent = nullptr) : QPushButton(parent)
    {
        setAttribute(Qt::WA_Hover, true);
        // 覆盖全局 QSS 的静态 hover，背景改由 paintEvent 绘制动画。
        setStyleSheet("QPushButton:hover{background:transparent;}"
                      "QPushButton:pressed{background:rgba(255,255,255,12);}");
        animation.setDuration(120);
        animation.setEasingCurve(QEasingCurve::OutCubic);
        connect(&animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
            hover = value.toReal();
            update();
        });
        connect(this, &QPushButton::toggled, this, [this]{ updateStateIcon(); });
    }
    void setHoverProgress(qreal value) { hover = std::clamp(value, 0.0, 1.0); update(); }
    qreal hoverProgress() const { return hover; }
    void setHoverTint(const QColor &color, int alpha = 20)
    {
        hoverTint = color;
        hoverAlpha = alpha;
    }
    void setStateIcons(const QIcon &normal, const QIcon &active)
    {
        normalIcon = normal;
        activeIcon = active;
        updateStateIcon();
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        QPushButton::enterEvent(event);
        animateTo(1.0);
        updateStateIcon();
    }
    void leaveEvent(QEvent *event) override
    {
        QPushButton::leaveEvent(event);
        animateTo(0.0);
        updateStateIcon();
    }
    void hideEvent(QHideEvent *event) override
    {
        animation.stop();
        hover = 0;
        if (!normalIcon.isNull()) QPushButton::setIcon(normalIcon);
        QPushButton::hideEvent(event);
    }
    void paintEvent(QPaintEvent *event) override
    {
        if (hover > 0.01 && isEnabled() && !isChecked()) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(hoverTint.red(), hoverTint.green(), hoverTint.blue(),
                                    qRound(hoverAlpha * hover)));
            painter.drawRoundedRect(QRectF(rect()), 8, 8);
        }
        QPushButton::paintEvent(event);
    }

private:
    void updateStateIcon()
    {
        if (normalIcon.isNull()) return;
        const bool bright = isEnabled() && !activeIcon.isNull() && (isChecked() || underMouse());
        QPushButton::setIcon(bright ? activeIcon : normalIcon);
    }
    void animateTo(qreal target)
    {
        animation.stop();
        animation.setStartValue(hover);
        animation.setEndValue(target);
        animation.start();
    }
    QVariantAnimation animation;
    qreal hover = 0;
    QColor hoverTint = QColor(255, 255, 255);
    int hoverAlpha = 20;
    QIcon normalIcon;
    QIcon activeIcon;
};

// 标注工具是单选，用一块会滑动的浅色底块标出当前工具。
class ToolCapsule : public QWidget {
public:
    explicit ToolCapsule(QWidget *parent) : QWidget(parent), animation(this, "geometry")
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFixedSize(30, 30);
        animation.setDuration(160);
        animation.setEasingCurve(QEasingCurve::OutCubic);
    }
    void glideTo(const QRect &target, bool animate)
    {
        animation.stop();
        if (!animate) {
            setGeometry(target);
            return;
        }
        animation.setStartValue(geometry());
        animation.setEndValue(target);
        animation.start();
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(QColor(163, 240, 206, 42), 1));
        p.setBrush(QColor(163, 240, 206, 26));
        p.drawRoundedRect(box, 8, 8);
    }
private:
    QPropertyAnimation animation;
};

// 轻量颜色弹层：预设色 + 自定义颜色，避免打断整体风格的系统对话框。
class ColorPopup : public QWidget {
public:
    ColorPopup(const QColor &current, const QList<QColor> &recent,
               std::function<void(const QColor &)> onPick,
               std::function<void()> onCustom, QWidget *parent)
        : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    {
        setObjectName("colorPopup");
        setAttribute(Qt::WA_TranslucentBackground);
        auto *column = new QVBoxLayout(this);
        column->setContentsMargins(10, 9, 10, 9);
        column->setSpacing(7);

        auto *title = new QLabel("标注颜色");
        column->addWidget(title);

        auto *grid = new QGridLayout;
        grid->setSpacing(3);
        const QList<QColor> presets = {
            QColor("#000000"), QColor("#FFFFFF"), QColor("#FF5F57"), QColor("#FF9F0A"), QColor("#FFD60A"),
            QColor("#30D158"), QColor("#0A84FF"), QColor("#BF5AF2"), QColor("#8E8E93"), QColor("#A3F0CE")};
        int index = 0;
        for (const QColor &color : presets) {
            grid->addWidget(dot(color, color == current, onPick), index / 5, index % 5);
            ++index;
        }
        column->addLayout(grid);

        if (!recent.isEmpty()) {
            auto *caption = new QLabel("最近使用");
            column->addWidget(caption);
            auto *row = new QHBoxLayout;
            row->setSpacing(3);
            for (const QColor &color : recent) row->addWidget(dot(color, color == current, onPick));
            row->addStretch();
            column->addLayout(row);
        }

        auto *line = new QFrame;
        line->setFixedHeight(1);
        line->setStyleSheet("background:rgba(255,255,255,22);border:0;");
        column->addWidget(line);

        auto *more = new QPushButton("自定义颜色…");
        more->setObjectName("popupMore");
        more->setFixedHeight(26);
        more->setCursor(Qt::PointingHandCursor);
        more->setFocusPolicy(Qt::NoFocus);
        connect(more, &QPushButton::clicked, this, [this, onCustom]{
            close();
            if (onCustom) onCustom();
        });
        column->addWidget(more);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(frame, 12, 12);
        painter.fillPath(path, QColor("#141416"));
        painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

private:
    QPushButton *dot(const QColor &color, bool selected,
                     const std::function<void(const QColor &)> &onPick)
    {
        auto *button = new HoverButton(this);
        button->setFixedSize(26, 26);
        button->setIconSize({18, 18});
        button->setIcon(QIcon(colorDot(color, 18, selected)));
        button->setToolTip(color.name());
        button->setAccessibleName(color.name());
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QPushButton::clicked, this, [this, color, onPick]{
            if (onPick) onPick(color);
            close();
        });
        return button;
    }
};

// 选中标注工具后自动浮出的颜色条，省掉“先点颜色再画”的步骤。
class ColorStrip : public QWidget {
public:
    ColorStrip(std::function<void(const QColor &)> onPick,
               std::function<void()> onCustom,
               std::function<void(int)> onSizeChange,
               std::function<void(int)> onAlphaChange,
               std::function<void()> onLayoutChange, QWidget *parent)
        : QWidget(parent), pick(std::move(onPick)), onSize(std::move(onSizeChange)),
          onAlpha(std::move(onAlphaChange)), onLayout(std::move(onLayoutChange))
    {
        setAttribute(Qt::WA_TranslucentBackground);
        auto *column = new QVBoxLayout(this);
        column->setContentsMargins(7, 5, 7, 5);
        column->setSpacing(6);
        auto *rowHost = new QWidget(this);
        auto *row = new QHBoxLayout(rowHost);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(2);
        const QList<QColor> presets = {
            QColor("#000000"), QColor("#FFFFFF"), QColor("#FF5F57"), QColor("#FF9F0A"), QColor("#FFD60A"),
            QColor("#30D158"), QColor("#0A84FF"), QColor("#BF5AF2"), QColor("#8E8E93"), QColor("#A3F0CE")};
        for (const QColor &color : presets) {
            auto *button = new HoverButton(this);
            button->setFixedSize(24, 24);
            button->setIconSize({18, 18});
            button->setToolTip(color.name());
            button->setAccessibleName(color.name());
            button->setCursor(Qt::PointingHandCursor);
            button->setFocusPolicy(Qt::NoFocus);
            connect(button, &QPushButton::clicked, this, [this, color]{
                setCurrent(color);
                if (pick) pick(color);
                hide();
            });
            dots.append({color, button});
            row->addWidget(button);
        }
        auto *more = new HoverButton(this);
        more->setFixedSize(24, 24);
        more->setIconSize({14, 14});
        more->setStateIcons(shapeIcon("plus", kIcon, 14), shapeIcon("plus", kMint, 14));
        more->setToolTip("自定义颜色");
        more->setAccessibleName("自定义颜色");
        more->setCursor(Qt::PointingHandCursor);
        more->setFocusPolicy(Qt::NoFocus);
        connect(more, &QPushButton::clicked, this, [this, onCustom]{
            hide();
            if (onCustom) onCustom();
        });
        row->addWidget(more);
        auto *styleToggle = new HoverButton(this);
        styleToggle->setFixedSize(24, 24);
        styleToggle->setIconSize({14, 14});
        styleToggle->setStateIcons(shapeIcon("sliders", kIcon, 14), shapeIcon("sliders", kMint, 14));
        styleToggle->setToolTip("粗细与透明度");
        styleToggle->setAccessibleName("粗细与透明度");
        styleToggle->setCursor(Qt::PointingHandCursor);
        styleToggle->setFocusPolicy(Qt::NoFocus);
        row->addWidget(styleToggle);

        styleRow = new QWidget(this);
        auto *styleLayout = new QHBoxLayout(styleRow);
        styleLayout->setContentsMargins(0, 0, 0, 0);
        styleLayout->setSpacing(6);
        const QString sliderStyle =
            "QSlider{background:transparent;height:16px;}"
            "QSlider::groove:horizontal{background:rgba(255,255,255,26);height:4px;border-radius:2px;}"
            "QSlider::sub-page:horizontal{background:#A3F0CE;border-radius:2px;}"
            "QSlider::handle:horizontal{background:#E9E9EC;width:12px;height:12px;"
            "margin:-5px 0;border-radius:6px;}";
        sizeSlider = new QSlider(Qt::Horizontal, styleRow);
        sizeSlider->setRange(1, 40);
        sizeSlider->setStyleSheet(sliderStyle);
        sizeSlider->setFixedWidth(110);
        sizeValue = new QLabel("4", styleRow);
        alphaSlider = new QSlider(Qt::Horizontal, styleRow);
        alphaSlider->setRange(30, 100);
        alphaSlider->setStyleSheet(sliderStyle);
        alphaSlider->setFixedWidth(110);
        alphaValue = new QLabel("100%", styleRow);
        auto *sizeCaption = new QLabel("粗细", styleRow);
        auto *alphaCaption = new QLabel("透明度", styleRow);
        for (QLabel *label : {sizeCaption, alphaCaption, sizeValue, alphaValue})
            label->setStyleSheet("color:#8E8E97;font-size:11px;background:transparent;");
        styleLayout->addWidget(sizeCaption);
        styleLayout->addWidget(sizeSlider);
        styleLayout->addWidget(sizeValue);
        styleLayout->addSpacing(8);
        styleLayout->addWidget(alphaCaption);
        styleLayout->addWidget(alphaSlider);
        styleLayout->addWidget(alphaValue);
        styleRow->setVisible(false);
        column->addWidget(rowHost);
        column->addWidget(styleRow);
        connect(sizeSlider, &QSlider::valueChanged, this, [this](int value){
            sizeValue->setNum(value);
            if (onSize) onSize(value);
        });
        connect(alphaSlider, &QSlider::valueChanged, this, [this](int value){
            alphaValue->setText(QString::number(value) + QLatin1Char('%'));
            if (onAlpha) onAlpha(value);
        });
        connect(styleToggle, &QPushButton::clicked, this, [this]{
            styleRow->setVisible(!styleRow->isVisible());
            adjustSize();
            if (onLayout) onLayout();
        });
        setCurrent(Qt::black);
    }

    void setStyleValues(int size, int alpha)
    {
        sizeSlider->blockSignals(true);
        alphaSlider->blockSignals(true);
        sizeSlider->setValue(std::clamp(size, 1, 40));
        alphaSlider->setValue(std::clamp(alpha, 30, 100));
        sizeValue->setNum(std::clamp(size, 1, 40));
        alphaValue->setText(QString::number(std::clamp(alpha, 30, 100)) + QLatin1Char('%'));
        sizeSlider->blockSignals(false);
        alphaSlider->blockSignals(false);
    }

    void setCurrent(const QColor &value)
    {
        current = value;
        for (const auto &[dotColor, button] : dots)
            button->setIcon(QIcon(colorDot(dotColor, 18, dotColor == value)));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QLinearGradient fill(0, body.top(), 0, body.bottom());
        fill.setColorAt(0.0, QColor(32, 32, 38, 248));
        fill.setColorAt(1.0, QColor(15, 15, 18, 248));
        p.setPen(QPen(QColor(255, 255, 255, 32), 1));
        p.setBrush(fill);
        p.drawRoundedRect(body, 11, 11);
    }

private:
    QColor current = Qt::black;
    QList<QPair<QColor, HoverButton *>> dots;
    std::function<void(const QColor &)> pick;
    std::function<void(int)> onSize;
    std::function<void(int)> onAlpha;
    std::function<void()> onLayout;
    QWidget *styleRow = nullptr;
    QSlider *sizeSlider = nullptr;
    QSlider *alphaSlider = nullptr;
    QLabel *sizeValue = nullptr;
    QLabel *alphaValue = nullptr;
};

// 最近截图缩略图弹层，最多保留五张。
class ShotPopup : public QWidget {
public:
    ShotPopup(const QList<QPair<QPixmap, QString>> &shots,
              std::function<void(int)> onPick, QWidget *parent)
        : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    {
        setObjectName("shotPopup");
        setAttribute(Qt::WA_TranslucentBackground);
        auto *column = new QVBoxLayout(this);
        column->setContentsMargins(12, 11, 12, 12);
        column->setSpacing(8);

        auto *title = new QLabel("最近截图");
        column->addWidget(title);
        auto *grid = new QGridLayout;
        grid->setSpacing(7);
        for (int i = 0; i < shots.size(); ++i) {
            auto *button = new QPushButton;
            button->setObjectName("shotThumb");
            button->setFixedSize(104, 68);
            button->setIconSize({98, 62});
            button->setIcon(QIcon(shots.at(i).first));
            button->setToolTip(shots.at(i).second);
            button->setCursor(Qt::PointingHandCursor);
            button->setFocusPolicy(Qt::NoFocus);
            connect(button, &QPushButton::clicked, this, [this, i, onPick]{
                if (onPick) onPick(i);
                close();
            });
            grid->addWidget(button, i / 3, i % 3);
        }
        column->addLayout(grid);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(frame, 12, 12);
        painter.fillPath(path, QColor("#141416"));
        painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
};

QFrame *separator(int height)
{
    auto *line = new QFrame;
    line->setFixedSize(1, height);
    line->setStyleSheet("background:rgba(255,255,255,36);border:0;");
    return line;
}

Qt::Edges edgeHit(const QSize &size, const QPoint &position, int margin = 7)
{
    Qt::Edges edges;
    if (position.x() <= margin) edges |= Qt::LeftEdge;
    else if (position.x() >= size.width() - margin) edges |= Qt::RightEdge;
    if (position.y() <= margin) edges |= Qt::TopEdge;
    else if (position.y() >= size.height() - margin) edges |= Qt::BottomEdge;
    return edges;
}

Qt::CursorShape cursorForEdges(Qt::Edges edges)
{
    const bool left = edges.testFlag(Qt::LeftEdge), right = edges.testFlag(Qt::RightEdge);
    const bool top = edges.testFlag(Qt::TopEdge), bottom = edges.testFlag(Qt::BottomEdge);
    if ((left && top) || (right && bottom)) return Qt::SizeFDiagCursor;
    if ((right && top) || (left && bottom)) return Qt::SizeBDiagCursor;
    if (left || right) return Qt::SizeHorCursor;
    return Qt::SizeVerCursor;
}

QPalette darkPalette()
{
    QPalette p;
    p.setColor(QPalette::Window, QColor("#141416"));
    p.setColor(QPalette::WindowText, QColor("#F4F4F5"));
    p.setColor(QPalette::Base, QColor("#0E0E11"));
    p.setColor(QPalette::AlternateBase, QColor("#161618"));
    p.setColor(QPalette::Text, QColor("#F4F4F5"));
    p.setColor(QPalette::Button, QColor("#1B1B1F"));
    p.setColor(QPalette::ButtonText, QColor("#E9E9EC"));
    p.setColor(QPalette::Highlight, kMint);
    p.setColor(QPalette::HighlightedText, QColor("#07120E"));
    p.setColor(QPalette::ToolTipBase, QColor("#1B1B1F"));
    p.setColor(QPalette::ToolTipText, QColor("#E9E9EC"));
    p.setColor(QPalette::PlaceholderText, QColor("#8E8E97"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#6B6B73"));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#6B6B73"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#6B6B73"));
    return p;
}

class TextDialog : public QDialog {
public:
    TextDialog(QWidget *parent, const QColor &color, const QPoint &anchor) : QDialog(parent)
    {
        setObjectName("textPopup");
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        setFixedWidth(360);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 15, 16, 16);
        layout->setSpacing(0);

        auto *header = new QHBoxLayout;
        auto *title = new QLabel("文字标注");
        title->setObjectName("tpTitle");
        header->addWidget(title);
        header->addStretch();
        auto *close = new QPushButton("✕");
        close->setObjectName("tpClose");
        close->setFixedSize(24, 24);
        close->setCursor(Qt::PointingHandCursor);
        close->setFocusPolicy(Qt::NoFocus);
        close->setAccessibleName("关闭文字输入");
        connect(close, &QPushButton::clicked, this, &QDialog::reject);
        header->addWidget(close);
        layout->addLayout(header);
        layout->addSpacing(10);

        edit = new QPlainTextEdit;
        edit->setObjectName("tpEdit");
        edit->setPlaceholderText("输入文字… Shift+Enter 换行");
        edit->setFixedHeight(96);
        edit->setTabChangesFocus(true);
        edit->installEventFilter(this);
        layout->addWidget(edit);
        layout->addSpacing(10);

        auto *footer = new QHBoxLayout;
        footer->setSpacing(8);
        auto *swatch = new QLabel;
        swatch->setPixmap(colorDot(color, 14));
        swatch->setFixedSize(14, 14);
        swatch->setToolTip(color.name());
        auto *meta = new QLabel("0 字");
        meta->setObjectName("tpMeta");
        footer->addWidget(swatch);
        footer->addWidget(meta);
        footer->addStretch();
        auto *cancel = new QPushButton("取消");
        cancel->setObjectName("tpGhost");
        cancel->setFixedSize(64, 32);
        cancel->setCursor(Qt::PointingHandCursor);
        auto *add = new QPushButton("添加");
        add->setObjectName("tpAdd");
        add->setFixedSize(76, 32);
        add->setCursor(Qt::PointingHandCursor);
        add->setEnabled(false);
        footer->addWidget(cancel);
        footer->addWidget(add);
        layout->addLayout(footer);

        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(add, &QPushButton::clicked, this, [this]{ submit(); });
        connect(edit, &QPlainTextEdit::textChanged, this, [this, meta, add]{
            const QString value = edit->toPlainText();
            meta->setText(QString("%1 字").arg(value.size()));
            add->setEnabled(!value.trimmed().isEmpty());
        });

        setStyleSheet(R"(
            QDialog#textPopup { background:transparent; }
            QLabel#tpTitle { color:#F4F4F5; font-size:12.5px; font-weight:600; }
            QLabel#tpMeta { color:#7C7C85; font-size:10.5px; }
            QPlainTextEdit#tpEdit { background:rgba(255,255,255,7);
                border:1px solid rgba(255,255,255,20); border-radius:9px;
                color:#EDEDF0; font-size:12.5px; padding:8px;
                selection-background-color:rgba(163,240,206,90); }
            QPlainTextEdit#tpEdit:focus { border-color:rgba(163,240,206,90); }
            QPushButton#tpClose { background:transparent; border:0; border-radius:7px;
                color:#A6A6AE; font-size:13px; padding:0; }
            QPushButton#tpClose:hover { background:rgba(255,69,58,62); color:#FFFFFF; }
            QPushButton#tpGhost { background:rgba(255,255,255,10);
                border:1px solid rgba(255,255,255,18); border-radius:8px;
                color:#D2D2D8; font-size:11.5px; }
            QPushButton#tpGhost:hover { background:rgba(255,255,255,18); }
            QPushButton#tpAdd { background:#A3F0CE; border:0; border-radius:8px;
                color:#07120E; font-size:11.5px; font-weight:600; }
            QPushButton#tpAdd:hover { background:#B8F7DC; }
            QPushButton#tpAdd:pressed { background:#8FE3BE; }
            QPushButton#tpAdd:disabled { background:rgba(163,240,206,52);
                color:rgba(7,18,14,135); }
        )");
        adjustSize();
        place(anchor);
    }
    QString text() const { return edit->toPlainText(); }
protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == edit && event->type() == QEvent::KeyPress) {
            auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Escape) { reject(); return true; }
            if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
                && !(key->modifiers() & Qt::ShiftModifier)) {
                submit();
                return true;
            }
        }
        return QDialog::eventFilter(watched, event);
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(QColor(255, 255, 255, 26), 1));
        p.setBrush(QColor("#0C0C0F"));
        p.drawRoundedRect(card, 14, 14);
    }
    void showEvent(QShowEvent *event) override
    {
        QDialog::showEvent(event);
        edit->setFocus();
        edit->moveCursor(QTextCursor::End);
    }
private:
    QPlainTextEdit *edit = nullptr;

    void submit()
    {
        if (edit->toPlainText().trimmed().isEmpty()) {
            edit->setFocus();
            return;
        }
        accept();
    }

    void place(const QPoint &anchor)
    {
        QScreen *screen = QGuiApplication::screenAt(anchor);
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return;
        const QRect area = screen->availableGeometry();
        QPoint pos = anchor + QPoint(16, 18);
        if (pos.x() + width() > area.right() - 10) pos.setX(anchor.x() - width() - 16);
        if (pos.y() + height() > area.bottom() - 10) pos.setY(anchor.y() - height() - 18);
        pos.setX(std::clamp(pos.x(), area.left() + 10,
                            std::max(area.left() + 10, area.right() - width() - 10)));
        pos.setY(std::clamp(pos.y(), area.top() + 10,
                            std::max(area.top() + 10, area.bottom() - height() - 10)));
        move(pos);
    }
};

#ifdef LUMENSHOT_OCR
// 结果面板右下角的拉伸手柄：拖动改变面板大小。
class PanelGrip : public QWidget {
public:
    explicit PanelGrip(QWidget *target) : QWidget(target), target_(target)
    {
        setFixedSize(16, 16);
        setCursor(Qt::SizeFDiagCursor);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragging = true;
            start = event->globalPosition().toPoint();
            startSize = target_->size();
        }
        QWidget::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (dragging && event->buttons() & Qt::LeftButton) {
            const QPointF delta = event->globalPosition() - start;
            target_->resize(std::max(360, startSize.width() + int(delta.x())),
                            std::max(260, startSize.height() + int(delta.y())));
        }
        QWidget::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        dragging = false;
        QWidget::mouseReleaseEvent(event);
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setPen(QPen(QColor(255, 255, 255, 90), 1.4));
        for (int i = 1; i <= 3; ++i)
            p.drawLine(QPointF(15 - i * 4, 15), QPointF(15, 15 - i * 4));
    }
private:
    QWidget *target_;
    bool dragging = false;
    QPointF start;
    QSize startSize;
};

// OCR 结果窗：识别文字可修正后一键复制，风格与文字输入框一致。
class OcrResultPanel : public QWidget {
    Q_OBJECT
public:
    OcrResultPanel(const QString &text, QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoMousePropagation);
        auto *column = new QVBoxLayout(this);
        column->setContentsMargins(18, 16, 18, 16);
        column->setSpacing(10);
        auto *title = new QLabel("识别文字（可修正后复制 · 按住空白处拖动窗口）");
        column->addWidget(title);
        edit = new QPlainTextEdit;
        edit->setPlainText(text);
        edit->setMinimumSize(320, 200);
        edit->setStyleSheet(
            "QPlainTextEdit{background:rgba(255,255,255,8);color:#E9E9EC;"
            "border:1px solid rgba(255,255,255,22);border-radius:8px;padding:8px;"
            "font-size:13px;selection-background-color:rgba(163,240,206,60);}");
        column->addWidget(edit);
        auto *row = new QHBoxLayout;
        row->addStretch();
        auto *copy = new QPushButton("复制全部");
        auto *close = new QPushButton("关闭");
        for (QPushButton *button : {copy, close}) {
            button->setFixedHeight(32);
            button->setCursor(Qt::PointingHandCursor);
            row->addWidget(button);
        }
        column->addLayout(row);
        setStyleSheet(
            "QLabel{color:#8E8E97;font-size:11px;background:transparent;}"
            "QPushButton#ocrCopy{background:#A3F0CE;color:#07120E;border:0;border-radius:8px;"
            "padding:0 20px;font-size:12px;}"
            "QPushButton#ocrCopy:hover{background:#C4F7DE;}"
            "QPushButton#ocrClose{background:rgba(255,255,255,12);color:#E9E9EC;border:0;"
            "border-radius:8px;padding:0 20px;font-size:12px;}"
            "QPushButton#ocrClose:hover{background:rgba(255,255,255,22);}");
        copy->setObjectName("ocrCopy");
        close->setObjectName("ocrClose");
        grip = new PanelGrip(this);
        connect(copy, &QPushButton::clicked, this, [this]{
            QApplication::clipboard()->setText(edit->toPlainText());
            QToolTip::showText(QCursor::pos(), "已复制识别文字", this);
        });
        connect(close, &QPushButton::clicked, this, &OcrResultPanel::closed);
    }
    void popup(const QRect &card)
    {
        adjustSize();
        QPoint position = card.center() - QPoint(width() / 2, height() / 2);
        if (QWidget *host = parentWidget()) {
            position.setX(std::clamp(position.x(), 8, std::max(8, host->width() - width() - 8)));
            position.setY(std::clamp(position.y(), 8, std::max(8, host->height() - height() - 8)));
        }
        move(position);
        grip->move(width() - 18, height() - 18);
        grip->raise();
        show();
        raise();
        edit->moveCursor(QTextCursor::End);
        edit->setFocus();
    }
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (grip) grip->move(width() - 18, height() - 18);
    }
signals:
    void closed();
protected:
    // 结果窗按住空白处即可拖动；文本框和按钮自行消费事件，不会误触。
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
            dragging = true;
        }
        QWidget::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (dragging && event->buttons() & Qt::LeftButton)
            move(event->globalPosition().toPoint() - dragOffset);
        QWidget::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        dragging = false;
        QWidget::mouseReleaseEvent(event);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(255, 255, 255, 30), 1));
        p.setBrush(QColor(20, 20, 24, 248));
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
    }
private:
    QPlainTextEdit *edit = nullptr;
    bool dragging = false;
    QPoint dragOffset;
    PanelGrip *grip = nullptr;
};
#endif

class Preview : public QWidget {
    Q_OBJECT
public:
    Masks masks;
    MaskShape shape = MaskShape::Rectangle;
    QColor color = Qt::black;
    int toolSize = 12;
    bool editing = false, circle = false;
    bool scrim = false;
    int selectedStep = -1;
    bool selectable = false;
    bool floating = false;
    bool cardDraggable = false;
    QPolygonF penPoints;
    int alphaPercent = 100;
    bool textResizing = false;
    int textStartWidth = 0;
    int textPreviewWidth = 0;
    bool cropMode = false;
    QRectF cropArea;
    bool cropDragging = false;
    Qt::Edges cropHandle;
    QPointF cropGrab;
    QRectF cropStart;
    static constexpr int shadowPad = 18;
    Preview(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(330, 160); setMouseTracking(true); }
    void edit(std::function<void(Masks &)> change, bool background)
    {
        if (!background) {
            change(masks);
            update();
            emit edited();
            return;
        }
        auto *task = new QFutureWatcher<Masks>(this);
        auto *progress = new QProgressDialog("正在处理图像…", "取消", 0, 0, window());
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(250);
        progress->setValue(0);
        emit busy(true);
        connect(task, &QFutureWatcher<Masks>::finished, this, [this, task, progress]{
            if (!progress->wasCanceled()) masks = task->result();
            progress->deleteLater();
            task->deleteLater();
            emit busy(false);
            emit edited();
            update();
        });
        task->setFuture(QtConcurrent::run([model=masks, change]() mutable {
            change(model);
            return model;
        }));
    }
    // 裁剪模式：先给出整图边框，拖四周把手或框内自由调整，双击/Enter 应用。
    void beginCrop()
    {
        cropArea = QRectF(masks.image().rect());
        cropDragging = false;
        selectedStep = -1;
        cropMode = true;
        update();
    }
    void applyCrop()
    {
        cropDragging = false;
        const QRect pixels = cropArea.normalized().toAlignedRect().intersected(masks.image().rect());
        cropMode = false;
        if (pixels.width() < 4 || pixels.height() < 4) { update(); return; }
        edit([pixels](Masks &model){ model.crop(pixels); }, false);
        shape = MaskShape::Rectangle;
        editing = false;
        setCursor(cardDraggable ? Qt::OpenHandCursor : Qt::ArrowCursor);
        emit cropped(pixels);
        update();
    }
signals:
    void edited();
    void busy(bool value);
    void cropped(const QRect &pixels);
    void moved();
    void pressed();
    void picked(int index);
private:
    bool dragging = false;
    bool movingStep = false;
    bool cardMoving = false;
    QPointF moveStart, moveDelta;
    QPointF cardGrab, cardStart;
    QPointF start, end;
    QRect targetRect() const
    {
        if (floating) {
            const QRect area = rect().adjusted(shadowPad, shadowPad, -shadowPad, -shadowPad);
            const QSize image = masks.image().size();
            if (image.isNull())
                return area;
            return QRect(area.topLeft(), image).intersected(area);
        }
        QRect area = rect().adjusted(10, 10, -10, -10);
        QRect target(QPoint(), masks.image().size().scaled(area.size(), Qt::KeepAspectRatio));
        target.moveCenter(area.center());
        return target;
    }
    QPointF toPixels(QPointF point) const
    {
        const auto target = targetRect();
        const auto &image = masks.image();
        return {std::clamp((point.x()-target.x()) * image.width()/target.width(), 0., double(image.width())),
                std::clamp((point.y()-target.y()) * image.height()/target.height(), 0., double(image.height()))};
    }
    QRectF draft() const
    {
        QPointF delta = end-start;
        if (circle && shape == MaskShape::Ellipse) {
            const double side = std::min(std::abs(delta.x()), std::abs(delta.y()));
            delta = {std::copysign(side, delta.x()), std::copysign(side, delta.y())};
        }
        return QRectF(start, start+delta).normalized();
    }
    QRectF widgetCropRect() const
    {
        const QRect target = targetRect();
        const auto &image = masks.image();
        const qreal sx = target.width() / qreal(image.width());
        const qreal sy = target.height() / qreal(image.height());
        return {target.x() + cropArea.x() * sx, target.y() + cropArea.y() * sy,
                cropArea.width() * sx, cropArea.height() * sy};
    }
    Qt::Edges cropHitHandle(const QPointF &position) const
    {
        const QRectF frame = widgetCropRect();
        if (QLineF(position, frame.topLeft()).length() <= 10)
            return Qt::TopEdge | Qt::LeftEdge;
        if (QLineF(position, frame.topRight()).length() <= 10)
            return Qt::TopEdge | Qt::RightEdge;
        if (QLineF(position, frame.bottomLeft()).length() <= 10)
            return Qt::BottomEdge | Qt::LeftEdge;
        if (QLineF(position, frame.bottomRight()).length() <= 10)
            return Qt::BottomEdge | Qt::RightEdge;
        const qreal edge = 7;
        if (position.y() > frame.top() - edge && position.y() < frame.bottom() + edge) {
            if (qAbs(position.x() - frame.left()) <= edge) return Qt::LeftEdge;
            if (qAbs(position.x() - frame.right()) <= edge) return Qt::RightEdge;
        }
        if (position.x() > frame.left() - edge && position.x() < frame.right() + edge) {
            if (qAbs(position.y() - frame.top()) <= edge) return Qt::TopEdge;
            if (qAbs(position.y() - frame.bottom()) <= edge) return Qt::BottomEdge;
        }
        return {};
    }
    static Qt::CursorShape cropCursorFor(Qt::Edges edges)
    {
        if (edges == (Qt::TopEdge | Qt::LeftEdge) || edges == (Qt::BottomEdge | Qt::RightEdge))
            return Qt::SizeFDiagCursor;
        if (edges == (Qt::TopEdge | Qt::RightEdge) || edges == (Qt::BottomEdge | Qt::LeftEdge))
            return Qt::SizeBDiagCursor;
        if (edges == Qt::LeftEdge || edges == Qt::RightEdge) return Qt::SizeHorCursor;
        if (edges == Qt::TopEdge || edges == Qt::BottomEdge) return Qt::SizeVerCursor;
        return Qt::SizeAllCursor;
    }
protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::RightButton) {
            dragging = false;
            movingStep = false;
            cardMoving = false;
            selectedStep = -1;
            emit picked(-1);
            update();
            return;
        }
        if (event->button() != Qt::LeftButton ||
            !QRectF(targetRect()).contains(event->position())) return;
        emit pressed();
        const QPointF pixel = toPixels(event->position());
        if (cropMode) {
            const Qt::Edges hit = cropHitHandle(event->position());
            if (!hit && !widgetCropRect().adjusted(-8, -8, 8, 8).contains(event->position()))
                return;
            cropDragging = true;
            cropGrab = event->position();
            cropStart = cropArea;
            cropHandle = hit;
            setCursor(cropCursorFor(hit));
            return;
        }
        if (!editing) {
            if (selectable) {
                selectedStep = masks.hitTest(pixel);
                emit picked(selectedStep);
                if (selectedStep >= 0) {
                    movingStep = true;
                    moveStart = pixel;
                    moveDelta = {};
                    update();
                    return;
                }
            }
            if (cardDraggable) {
                cardMoving = true;
                cardGrab = event->globalPosition();
                cardStart = pos();
                setCursor(Qt::ClosedHandCursor);
                update();
                return;
            }
            selectedStep = -1;
            update();
            return;
        }
        // 画笔/马赛克笔刷：直接起笔，不做命中选择
        if (shape == MaskShape::Pen || shape == MaskShape::MosaicBrush) {
            penPoints.clear();
            penPoints.append(pixel);
            start = end = pixel;
            dragging = true;
            return;
        }
        // 绘制工具激活时，点在已有标注上直接拖动它，点在空白处才画新标注。
        if (shape != MaskShape::Crop) {
            const int hitIndex = masks.hitTest(pixel);
            if (hitIndex >= 0) {
                selectedStep = hitIndex;
                emit picked(hitIndex);
                // 文本宽度手柄：选中文字后按住右缘手柄横向拉伸
                if (masks.stepShape(hitIndex) == MaskShape::Text) {
                    const QRectF bounds = masks.stepBounds(hitIndex);
                    if (qAbs(pixel.x() - bounds.right()) <= 9 &&
                        qAbs(pixel.y() - bounds.center().y()) <= 12) {
                        textResizing = true;
                        textStartWidth = int(bounds.width());
                        textPreviewWidth = textStartWidth;
                        moveStart = pixel;
                        movingStep = false;
                        update();
                        return;
                    }
                }
                movingStep = true;
                moveStart = pixel;
                moveDelta = {};
                update();
                return;
            }
        }
        start = end = pixel;
        dragging = true;
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (cardMoving) {
            const QPoint delta = (event->globalPosition() - cardGrab).toPoint();
            QPoint next = (cardStart + delta).toPoint();
            if (QWidget *host = parentWidget()) {
                const int imageWidth = width() - 2 * shadowPad;
                const int imageHeight = height() - 2 * shadowPad;
                next.setX(std::clamp(next.x(), -shadowPad,
                                     std::max(-shadowPad, host->width() - imageWidth - shadowPad)));
                next.setY(std::clamp(next.y(), -shadowPad,
                                     std::max(-shadowPad, host->height() - imageHeight - shadowPad)));
            }
            move(next);
            emit moved();
            return;
        }
        if (movingStep) {
            moveDelta = toPixels(event->position()) - moveStart;
            update();
            return;
        }
        if (cropDragging) {
            const QPointF delta = event->position() - cropGrab;
            const QRect target = targetRect();
            const auto &img = masks.image();
            const QPointF deltaImg(delta.x() * img.width() / target.width(),
                                   delta.y() * img.height() / target.height());
            const QRectF bounds = QRectF(img.rect());
            QRectF next = cropStart;
            if (!cropHandle) {
                next.translate(deltaImg);
                next.moveLeft(std::clamp(next.left(), 0.0, bounds.width() - next.width()));
                next.moveTop(std::clamp(next.top(), 0.0, bounds.height() - next.height()));
            } else {
                if (cropHandle & Qt::LeftEdge)
                    next.setLeft(std::clamp(next.left() + deltaImg.x(), 0.0, next.right() - 8));
                if (cropHandle & Qt::RightEdge)
                    next.setRight(std::clamp(next.right() + deltaImg.x(), next.left() + 8, bounds.width()));
                if (cropHandle & Qt::TopEdge)
                    next.setTop(std::clamp(next.top() + deltaImg.y(), 0.0, next.bottom() - 8));
                if (cropHandle & Qt::BottomEdge)
                    next.setBottom(std::clamp(next.bottom() + deltaImg.y(), next.top() + 8, bounds.height()));
            }
            cropArea = next;
            update();
            return;
        }
        if (dragging && (shape == MaskShape::Pen || shape == MaskShape::MosaicBrush)) {
            const QPointF pixel = toPixels(event->position());
            if (penPoints.isEmpty() || QLineF(penPoints.last(), pixel).length() > 2)
                penPoints.append(pixel);
            update();
            return;
        }
        if (textResizing) {
            const int dx = int(toPixels(event->position()).x() - moveStart.x());
            const QRectF bounds = masks.stepBounds(selectedStep);
            textPreviewWidth = std::clamp(textStartWidth + dx, 60,
                                          masks.image().width() - int(bounds.left()));
            update();
            return;
        }
        if (dragging) {
            end = toPixels(event->position());
            update();
            return;
        }
        if (cropMode) {
            const QRectF frame = widgetCropRect();
            if (frame.adjusted(-8, -8, 8, 8).contains(event->position()))
                setCursor(cropCursorFor(cropHitHandle(event->position())));
            else
                setCursor(Qt::ArrowCursor);
            return;
        }
        if (!editing && selectable) {
            if (masks.hitTest(toPixels(event->position())) >= 0)
                setCursor(Qt::SizeAllCursor);
            else if (cardDraggable)
                setCursor(Qt::OpenHandCursor);
            else
                setCursor(Qt::ArrowCursor);
        }
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (cardMoving) {
            if (event->button() != Qt::LeftButton) return;
            cardMoving = false;
            setCursor(Qt::OpenHandCursor);
            return;
        }
        if (movingStep) {
            if (event->button() != Qt::LeftButton) return;
            movingStep = false;
            const QPointF delta = moveDelta;
            moveDelta = {};
            if (!delta.isNull() && selectedStep >= 0) {
                const int index = selectedStep;
                edit([index, delta](Masks &model){ model.moveStep(index, delta); }, masks.isLarge());
            } else {
                update();
            }
            return;
        }
        if (cropDragging) {
            if (event->button() != Qt::LeftButton) return;
            cropDragging = false;
            update();
            return;
        }
        if (textResizing) {
            if (event->button() != Qt::LeftButton) return;
            textResizing = false;
            const int index = selectedStep;
            const int width = textPreviewWidth;
            textPreviewWidth = 0;
            if (index >= 0) edit([index, width](Masks &m){ m.setStepTextWidth(index, width); },
                                 masks.isLarge());
            return;
        }
        if (dragging && (shape == MaskShape::Pen || shape == MaskShape::MosaicBrush)) {
            if (event->button() != Qt::LeftButton) return;
            dragging = false;
            if (penPoints.size() >= 2) {
                const MaskShape penShape = shape;
                const QColor penColor = color;
                const int penSize = toolSize;
                edit([penShape, pts = penPoints, penColor, penSize](Masks &m){
                    m.add(penShape, pts, penColor, penSize);
                }, masks.isLarge());
            }
            penPoints.clear();
            return;
        }
        if (!dragging || event->button() != Qt::LeftButton) return;
        end = toPixels(event->position());
        dragging = false;
        QString text;
        if (shape == MaskShape::Text) {
            TextDialog dialog(this, color, event->globalPosition().toPoint());
            if (dialog.exec() != QDialog::Accepted) { update(); return; }
            text = dialog.text();
            if (text.trimmed().isEmpty()) { update(); return; }
        } else if (shape == MaskShape::Number) {
            // 编号取画布上现有序号的最大值 +1，撤销或删除后不会跳号。
            text = QString::number(masks.maxNumberText() + 1);
        }
        const bool alongLine = shape == MaskShape::Arrow || shape == MaskShape::Line
            || shape == MaskShape::Highlight || shape == MaskShape::Text
            || shape == MaskShape::Number;
        auto bounds = alongLine ? QRectF(start, end) : draft();
        const int alphaOverride = shape == MaskShape::Rectangle || shape == MaskShape::Ellipse ||
                                  shape == MaskShape::RoundedRect || shape == MaskShape::Bubble
                                  ? alphaPercent : -1;
        edit([shape=shape, bounds, color=color, size=toolSize, text, alphaOverride](Masks &model){
            model.add(shape, bounds, color, size, text, alphaOverride);
        }, masks.isLarge());
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (cropMode && cropArea.contains(toPixels(event->position()))) {
            applyCrop();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }
    void paintSelection(QPainter &p)
    {
        if (selectedStep < 0 || selectedStep >= masks.stepCount()) return;
        const QImage &image = masks.image();
        if (image.isNull()) return;
        const QRect target = targetRect();
        QRectF pixels = masks.stepBounds(selectedStep);
        if (movingStep) pixels.translate(moveDelta);
        const qreal sx = target.width() / qreal(image.width());
        const qreal sy = target.height() / qreal(image.height());
        const QRectF box(target.x() + pixels.x() * sx, target.y() + pixels.y() * sy,
                         pixels.width() * sx, pixels.height() * sy);
        p.setPen(QPen(QColor(163, 240, 206, 220), 1.4, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(box.adjusted(-4, -4, 4, 4), 5, 5);
        for (const QPointF &point : {box.topLeft(), box.topRight(), box.bottomLeft(), box.bottomRight()}) {
            p.setPen(QPen(QColor(8, 10, 12, 200), 1));
            p.setBrush(QColor("#A3F0CE"));
            p.drawEllipse(point, 3.2, 3.2);
        }
        if (movingStep) return;
        const QString hint = "拖动移动 · Delete 删除";
        QFont hintFont = font();
        hintFont.setPixelSize(10);
        const QFontMetrics metrics(hintFont);
        const qreal hintWidth = metrics.horizontalAdvance(hint) + 16;
        const qreal hintHeight = 20;
        QPointF position(box.left(), box.top() - hintHeight - 7);
        if (position.y() < 6) position.setY(box.bottom() + 7);
        position.setX(std::clamp(position.x(), 6.0, std::max(6.0, width() - hintWidth - 6)));
        const QRectF pill(position, QSizeF(hintWidth, hintHeight));
        p.setPen(QPen(QColor(255, 255, 255, 28), 1));
        p.setBrush(QColor(10, 10, 12, 220));
        p.drawRoundedRect(pill, 6, 6);
        p.setFont(hintFont);
        p.setPen(QColor("#C9C9CF"));
        p.drawText(pill, Qt::AlignCenter, hint);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        const auto &image = masks.image();

        if (image.isNull()) {
            const QRectF content = QRectF(rect()).adjusted(0, 0, 0, -56);
            const QPointF center = content.center() - QPointF(0, 16);
            const qreal groupHeight = 68 + 18 + 28 + 10 + 22;
            const qreal top = center.y() - groupHeight / 2;
            const QRectF orb(center.x() - 34, top, 68, 68);

            QRadialGradient glow(orb.center(), 190);
            glow.setColorAt(0.0, QColor(163, 240, 206, 24));
            glow.setColorAt(0.55, QColor(163, 240, 206, 8));
            glow.setColorAt(1.0, QColor(163, 240, 206, 0));
            p.fillRect(rect(), glow);

            QRadialGradient orbFill(orb.center(), orb.width() / 2);
            orbFill.setColorAt(0.0, QColor(163, 240, 206, 42));
            orbFill.setColorAt(0.62, QColor(163, 240, 206, 12));
            orbFill.setColorAt(1.0, QColor(163, 240, 206, 4));
            p.setPen(QPen(QColor(163, 240, 206, 40), 1));
            p.setBrush(orbFill);
            p.drawEllipse(orb);
            p.drawPixmap(QPointF(orb.center().x() - 14, orb.center().y() - 14),
                         tinted("focus", kMint, 28));

            QFont title = font();
            title.setPixelSize(19);
            title.setWeight(QFont::Medium);
            p.setFont(title);
            p.setPen(QColor("#F4F4F5"));
            const QRectF titleRect(0, orb.bottom() + 18, width(), 28);
            p.drawText(titleRect, Qt::AlignHCenter | Qt::AlignTop, "还没有截图");

            QFont sub = font();
            sub.setPixelSize(12);
            QFont keyFont = sub;
            keyFont.setPixelSize(11);
            const QString lead = "选择一种方式开始，或按";
            const QString key = "F1";
            const QFontMetrics leadMetrics(sub), keyMetrics(keyFont);
            const int gap = 6, padX = 7, rowHeight = 22;
            const int total = leadMetrics.horizontalAdvance(lead) + gap
                            + keyMetrics.horizontalAdvance(key) + padX * 2;
            const qreal y = titleRect.bottom() + 10;
            qreal x = (width() - total) / 2.0;
            p.setFont(sub);
            p.setPen(QColor("#8A8A93"));
            p.drawText(QRectF(x, y, leadMetrics.horizontalAdvance(lead), rowHeight),
                       Qt::AlignLeft | Qt::AlignVCenter, lead);
            const QRectF keyRect(x + leadMetrics.horizontalAdvance(lead) + gap, y + 1,
                                 keyMetrics.horizontalAdvance(key) + padX * 2, rowHeight - 2);
            p.setPen(QPen(QColor(255, 255, 255, 26), 1));
            p.setBrush(QColor(255, 255, 255, 10));
            p.drawRoundedRect(keyRect, 5, 5);
            p.setFont(keyFont);
            p.setPen(QColor("#D6D6DB"));
            p.drawText(keyRect, Qt::AlignCenter, key);
            return;
        }

        const QRect target = targetRect();
        QPainterPath clip;
        if (floating) {
            const int radius = 4;
            for (int i = 12; i >= 1; --i) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0, 0, 0, 7 + (12 - i) * 2));
                p.drawRoundedRect(QRectF(target).adjusted(-i, -i + 2, i, i + 2),
                                  radius + i, radius + i);
            }
            clip.addRoundedRect(QRectF(target), radius, radius);
            p.save();
            p.setClipPath(clip);
            p.drawImage(target, image);
            p.restore();
            p.setPen(QPen(QColor(255, 255, 255, 40), 1));
            p.setBrush(Qt::NoBrush);
            p.drawPath(clip);
        } else {
            const QRectF paper = QRectF(target).adjusted(-3, -3, 3, 3);
            for (int i = 9; i >= 1; --i) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0, 0, 0, 5 + (9 - i)));
                p.drawRoundedRect(paper.adjusted(-i, -i + 1, i, i + 1), 11 + i, 11 + i);
            }
            p.setBrush(QColor("#19191D"));
            p.setPen(QPen(QColor(255, 255, 255, 20), 1));
            p.drawRoundedRect(paper, 12, 12);

            clip.addRoundedRect(QRectF(target), 9, 9);
            p.save();
            p.setClipPath(clip);
            p.drawImage(target, image);
            p.restore();
            p.setPen(QPen(QColor(255, 255, 255, 28), 1));
            p.setBrush(Qt::NoBrush);
            p.drawPath(clip);
        }

        if (cropMode) {
            // 裁剪边框：框外压暗，三分参考线，四角方块与四边中点短柄可拖。
            const QRectF frame = widgetCropRect();
            QRegion dim(target);
            dim -= frame.toRect();
            p.save();
            p.setClipRegion(dim);
            p.fillRect(target, QColor(4, 7, 9, 128));
            p.restore();
            p.setPen(QPen(QColor(255, 255, 255, 60), 1, Qt::DotLine));
            p.setBrush(Qt::NoBrush);
            for (int i = 1; i <= 2; ++i) {
                p.drawLine(QPointF(frame.left() + frame.width() * i / 3, frame.top()),
                           QPointF(frame.left() + frame.width() * i / 3, frame.bottom()));
                p.drawLine(QPointF(frame.left(), frame.top() + frame.height() * i / 3),
                           QPointF(frame.right(), frame.top() + frame.height() * i / 3));
            }
            p.setPen(QPen(QColor(255, 255, 255, 235), 1.4));
            p.drawRect(frame);
            p.setPen(QPen(QColor(10, 12, 14, 220), 1));
            p.setBrush(QColor(255, 255, 255, 235));
            const qreal mark = 4;
            for (const QPointF &corner : {frame.topLeft(), frame.topRight(),
                                          frame.bottomLeft(), frame.bottomRight()})
                p.drawRect(QRectF(corner.x() - mark, corner.y() - mark, mark * 2, mark * 2));
            const QPointF center = frame.center();
            p.drawRect(QRectF(frame.left() - 1, center.y() - 9, 2, 18));
            p.drawRect(QRectF(frame.right() - 1, center.y() - 9, 2, 18));
            p.drawRect(QRectF(center.x() - 9, frame.top() - 1, 18, 2));
            p.drawRect(QRectF(center.x() - 9, frame.bottom() - 1, 18, 2));
        }

        if (scrim) {
            // 顶部/底部渐变，保证悬浮条在亮色截图上依然清晰。
            const qreal radius = window() && window()->isMaximized() ? 0 : 14;
            QPainterPath frame;
            frame.addRoundedRect(QRectF(rect()), radius, radius);
            p.save();
            p.setClipPath(frame);
            QLinearGradient top(0, 0, 0, 104);
            top.setColorAt(0, QColor(0, 0, 0, 118));
            top.setColorAt(1, QColor(0, 0, 0, 0));
            p.fillRect(QRectF(0, 0, width(), 104), top);
            QLinearGradient bottom(0, height() - 126, 0, height());
            bottom.setColorAt(0, QColor(0, 0, 0, 0));
            bottom.setColorAt(1, QColor(0, 0, 0, 150));
            p.fillRect(QRectF(0, height() - 126, width(), 126), bottom);
            p.restore();
        }

        if (dragging && (shape == MaskShape::Pen || shape == MaskShape::MosaicBrush)
            && penPoints.size() >= 2) {
            p.save();
            p.setClipPath(clip);
            p.translate(target.topLeft());
            p.scale(double(target.width())/image.width(), double(target.height())/image.height());
            QPainterPath stroke;
            stroke.moveTo(penPoints.first());
            for (int i = 1; i < penPoints.size(); ++i) stroke.lineTo(penPoints.at(i));
            if (shape == MaskShape::Pen) {
                p.setPen(QPen(color, std::max(1, toolSize), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                p.setBrush(Qt::NoBrush);
            } else {
                // 马赛克笔刷预览用实线半透明带，读起来才是连续路径
                p.setPen(QPen(QColor(255, 255, 255, 130), std::max(2, toolSize), Qt::SolidLine,
                              Qt::RoundCap, Qt::RoundJoin));
                p.setBrush(Qt::NoBrush);
            }
            p.drawPath(stroke);
            p.restore();
        }

        if (dragging) {
            p.save();
            p.setClipPath(clip);
            p.translate(target.topLeft());
            p.scale(double(target.width())/image.width(), double(target.height())/image.height());
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            if (shape == MaskShape::Arrow || shape == MaskShape::Line || shape == MaskShape::Highlight) {
                QColor ink = color;
                if (shape == MaskShape::Highlight) ink.setAlpha(110);
                p.setPen(QPen(ink, std::max(1, toolSize), Qt::SolidLine, Qt::RoundCap));
                p.drawLine(start, end);
            } else if (shape == MaskShape::Number) {
                p.setPen(QPen(QColor(255, 255, 255, 150), 1));
                p.setBrush(color);
                p.drawEllipse(start, toolSize / 2.0, toolSize / 2.0);
            } else if (shape == MaskShape::Text) {
                p.setPen(QPen(color, 1));
                p.drawLine(start,start+QPointF(0,toolSize));
            } else if (shape == MaskShape::Blur || shape == MaskShape::Mosaic) {
                p.setPen(QPen(Qt::white, 0, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawRect(draft());
            } else if (shape == MaskShape::Ellipse) p.drawEllipse(draft());
            else p.drawRect(draft());
            p.restore();

            QString text;
            if (shape == MaskShape::Arrow || shape == MaskShape::Line || shape == MaskShape::Highlight) {
                const QPointF delta = end - start;
                text = QString("%1 px").arg(qRound(std::hypot(delta.x(), delta.y())));
            } else if (shape != MaskShape::Text && shape != MaskShape::Number) {
                const QRectF area = draft();
                text = QString("%1 × %2").arg(qRound(area.width())).arg(qRound(area.height()));
            }
            if (!text.isEmpty() && image.width() > 0 && image.height() > 0) {
                QFont hudFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
                hudFont.setPixelSize(11);
                const QFontMetrics metrics(hudFont);
                const qreal badgeWidth = metrics.horizontalAdvance(text) + 18;
                const qreal badgeHeight = 22;
                const QPointF tip(target.x() + end.x() * target.width() / image.width(),
                                  target.y() + end.y() * target.height() / image.height());
                QPointF position(tip.x() + 14, tip.y() + 14);
                if (position.x() + badgeWidth > target.right()) position.setX(tip.x() - badgeWidth - 14);
                if (position.y() + badgeHeight > target.bottom()) position.setY(tip.y() - badgeHeight - 14);
                const qreal maxX = std::max(qreal(target.left()), qreal(target.right()) - badgeWidth);
                const qreal maxY = std::max(qreal(target.top()), qreal(target.bottom()) - badgeHeight);
                position.setX(std::clamp(position.x(), qreal(target.left()), maxX));
                position.setY(std::clamp(position.y(), qreal(target.top()), maxY));
                const QRectF badge(position, QSizeF(badgeWidth, badgeHeight));
                p.setPen(QPen(QColor(255, 255, 255, 34), 1));
                p.setBrush(QColor(10, 10, 12, 224));
                p.drawRoundedRect(badge, 6, 6);
                p.setFont(hudFont);
                p.setPen(QColor("#E9E9EC"));
                p.drawText(badge, Qt::AlignCenter, text);
            }
        }
        paintSelection(p);
    }
};

QString imageFormat(const QString &suffix)
{
    const QString value = suffix.toLower();
    if (value == "jpg" || value == "jpeg") return "JPEG";
    if (value == "webp") return "WEBP";
    return "PNG";
}

bool saveImage(QWidget *parent, QImage image)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop");
    QString format = settings.value("saveFormat", "PNG").toString().toUpper();
    if (format == "JPG") format = "JPEG";
    if (format != "JPEG" && format != "WEBP") format = "PNG";
    const QString suffix = format == "JPEG" ? "jpg" : format.toLower();

    QFileDialog dialog(parent, "保存截图");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters({"PNG 图片 (*.png)", "JPEG 图片 (*.jpg *.jpeg)", "WebP 图片 (*.webp)"});
    dialog.selectNameFilter(format == "JPEG" ? "JPEG 图片 (*.jpg *.jpeg)"
                            : format == "WEBP" ? "WebP 图片 (*.webp)" : "PNG 图片 (*.png)");
    dialog.setDefaultSuffix(suffix);
    dialog.setDirectory(saveDirectory());
    dialog.selectFile("Lumenshot-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + "." + suffix);
    if (!dialog.exec()) return false;

    QString path = dialog.selectedFiles().value(0);
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += "." + suffix;
    const QString chosen = imageFormat(QFileInfo(path).suffix());
    const int quality = chosen == "PNG" ? -1 : 92;

    QProgressDialog progress(QString("正在保存 %1…").arg(chosen), "取消", 0, 0, parent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(200);
    progress.setValue(0);
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QFutureWatcher<bool> watcher;
        QEventLoop loop;
        QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
        watcher.setFuture(QtConcurrent::run([&file, image, chosen, quality]{
            return image.save(&file, chosen.toLatin1().constData(), quality);
        }));
        loop.exec();
        watcher.waitForFinished();
        if (progress.wasCanceled()) return false;
        if (watcher.result() && file.commit()) {
            settings.setValue("saveFormat", chosen);
            settings.setValue("saveDirectory", QFileInfo(path).absolutePath());
            return true;
        }
    }
    progress.hide();
    QMessageBox::warning(parent, "保存失败", "请检查目标目录的写入权限或选择其他格式。");
    return false;
}

QImage decoratedImage(const QImage &source)
{
    if (source.isNull()) return {};
    QImage image = source;
    if (image.width() > 1600) image = image.scaledToWidth(1600, Qt::SmoothTransformation);
    const int pad = qMax(36, qRound(image.width() * 0.065));
    const int footer = 42;
    const QSize out(image.width() + pad * 2, image.height() + pad * 2 + footer);
    QImage canvas(out, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QLinearGradient background(0, 0, out.width(), out.height());
    background.setColorAt(0.0, QColor("#13232E"));
    background.setColorAt(0.55, QColor("#0B1319"));
    background.setColorAt(1.0, QColor("#070D11"));
    p.fillRect(canvas.rect(), background);
    QRadialGradient glow(QPointF(out.width() * 0.22, 0), out.width() * 0.9);
    glow.setColorAt(0.0, QColor(163, 240, 206, 40));
    glow.setColorAt(1.0, QColor(163, 240, 206, 0));
    p.fillRect(canvas.rect(), glow);

    const QRectF card(pad, pad, image.width(), image.height());
    p.setPen(Qt::NoPen);
    for (int i = 12; i >= 1; --i) {
        p.setBrush(QColor(0, 0, 0, 5 + (12 - i)));
        p.drawRoundedRect(card.adjusted(-i, -i + 2, i, i + 2), 18 + i, 18 + i);
    }
    QPainterPath clip;
    clip.addRoundedRect(card, 18, 18);
    p.save();
    p.setClipPath(clip);
    p.drawImage(card.topLeft(), image);
    p.restore();
    p.setPen(QPen(QColor(255, 255, 255, 34), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(clip);

    QFont mark = p.font();
    mark.setPixelSize(11);
    p.setFont(mark);
    p.setPen(QColor(255, 255, 255, 76));
    p.drawText(QRectF(pad, out.height() - footer, out.width() - pad * 2, footer),
               Qt::AlignRight | Qt::AlignVCenter, "Lumenshot");
    return canvas;
}

class PinnedPreview;
QList<QPointer<PinnedPreview>> &livePins();

class PinnedPreview : public Preview {
public:
    PinnedPreview()
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setFocusPolicy(Qt::StrongFocus);
        livePins().append(this);
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(true);
        resize(460, 340);
        setMinimumSize(170, 130);

        bar->setObjectName("pinnedBar");
        bar->setAttribute(Qt::WA_StyledBackground, true);
        bar->setStyleSheet(
            "#pinnedBar{background:rgba(14,14,17,228);border:1px solid rgba(255,255,255,32);"
            "border-radius:11px;}"
            "#pinnedBar QPushButton{background:transparent;border:0;border-radius:8px;padding:0;}");
        auto *row = new QHBoxLayout(bar);
        row->setContentsMargins(4, 4, 4, 4);
        row->setSpacing(2);

        copyButton = pinButton("copy", "复制图片");
        save = pinButton("download", "保存图片");
        toggle = pinButton("pin-off", "取消置顶");
        closeButton = pinButton(QString(), "关闭预览");
        closeButton->setIcon(QIcon(controlGlyph("close", QColor("#C9C9CF"))));
        static_cast<HoverButton *>(closeButton)->setHoverTint(QColor(255, 69, 58), 62);
        for (auto *button : {copyButton, save, toggle, closeButton}) row->addWidget(button);
        bar->adjustSize();
        bar->hide();

        sizeTag = new QLabel(this);
        sizeTag->setObjectName("pinnedSize");
        sizeTag->setAttribute(Qt::WA_TransparentForMouseEvents);
        sizeTag->setStyleSheet(
            "background:rgba(14,14,17,215);border:1px solid rgba(255,255,255,28);"
            "border-radius:8px;color:#C9C9CF;font-size:10px;padding:3px 7px;");
        sizeTag->hide();

        barOpacity = new QGraphicsOpacityEffect(bar);
        barOpacity->setOpacity(0.0);
        bar->setGraphicsEffect(barOpacity);
        sizeOpacity = new QGraphicsOpacityEffect(sizeTag);
        sizeOpacity->setOpacity(0.0);
        sizeTag->setGraphicsEffect(sizeOpacity);
        chromeFade = new QVariantAnimation(this);
        chromeFade->setDuration(150);
        chromeFade->setEasingCurve(QEasingCurve::OutCubic);
        connect(chromeFade, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
            const qreal opacity = value.toReal();
            barOpacity->setOpacity(opacity);
            sizeOpacity->setOpacity(opacity);
        });
        connect(chromeFade, &QVariantAnimation::finished, this, [this]{
            if (!chromeVisible) {
                bar->hide();
                sizeTag->hide();
            }
        });
        windowFade = new QPropertyAnimation(this, "windowOpacity", this);
        windowFade->setDuration(160);
        windowFade->setEasingCurve(QEasingCurve::OutCubic);

        connect(copyButton, &QPushButton::clicked, this, [this]{
            QApplication::clipboard()->setImage(masks.image());
            QToolTip::showText(copyButton->mapToGlobal(copyButton->rect().bottomLeft()),
                               "已复制到剪贴板", copyButton);
        });
        connect(save, &QPushButton::clicked, this, [this]{ saveImage(this, masks.image()); });
        connect(toggle, &QPushButton::clicked, this, [this]{ setPinned(!isPinned()); });
        connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
        connect(qApp, &QApplication::focusChanged, this, [this]{ syncToolbar(); });
    }

    void present(const QImage &snapshot)
    {
        const bool wasVisible = isVisible();
        masks.reset(snapshot);
        QSize target = snapshot.size();
        target.scale(QSize(560, 420), Qt::KeepAspectRatio);
        resize(target + QSize(20, 20));
        if (!wasVisible) setWindowOpacity(0.0);
        setPinned(true);
        showNormal();
        raise();
        sizeTag->setText(QString("%1 × %2").arg(snapshot.width()).arg(snapshot.height()));
        sizeTag->adjustSize();
        placeChildren();
        flashToolbar();
        update();
        if (!wasVisible) {
            windowFade->stop();
            windowFade->setStartValue(0.0);
            windowFade->setEndValue(1.0);
            windowFade->start();
        }
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(frame, 12, 12);
        painter.fillPath(path, QColor("#0B0B0D"));
        painter.setPen(QPen(QColor(255, 255, 255, 90), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
        painter.end();
        Preview::paintEvent(event);
    }
    void resizeEvent(QResizeEvent *event) override
    {
        Preview::resizeEvent(event);
        placeChildren();
    }
    void enterEvent(QEnterEvent *event) override { Preview::enterEvent(event); showChrome(); }
    void leaveEvent(QEvent *event) override { Preview::leaveEvent(event); syncToolbar(); }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && windowHandle()) {
            const Qt::Edges edges = edgeHit(size(), event->position().toPoint());
            if (edges) windowHandle()->startSystemResize(edges);
            else windowHandle()->startSystemMove();
            return;
        }
        Preview::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        setCursor(cursorForEdges(edgeHit(size(), event->position().toPoint())));
        Preview::mouseMoveEvent(event);
    }

    void zoomAt(const QPoint &cursorGlobal, qreal factor)
    {
        const QSize img = masks.image().size();
        if (img.isEmpty()) return;
        const qreal nextScale = std::clamp(zoomScale * factor, 0.25, 6.0);
        if (qFuzzyCompare(nextScale, zoomScale)) return;
        const QRect oldFrame = frameGeometry();
        const QSize newSize(std::max(140, int(img.width() * nextScale) + 20),
                            std::max(100, int(img.height() * nextScale) + 20));
        const qreal s = qreal(newSize.width()) / std::max(1, oldFrame.width());
        const QPoint topLeft = cursorGlobal - QPoint(
            int((cursorGlobal.x() - oldFrame.x()) * s),
            int((cursorGlobal.y() - oldFrame.y()) * s));
        zoomScale = nextScale;
        setGeometry(QRect(topLeft, newSize));
    }
    void wheelEvent(QWheelEvent *event) override
    {
        const QPoint delta = event->angleDelta();
        if (delta.y() == 0) { event->ignore(); return; }
        if (event->modifiers() & Qt::ControlModifier) {
            setWindowOpacity(std::clamp(windowOpacity() + (delta.y() > 0 ? 0.05 : -0.05),
                                        0.3, 1.0));
            showOpacityTag();
            event->accept();
            return;
        }
        const qreal factor = delta.y() > 0 ? 1.1 : 1.0 / 1.1;
        zoomAt(event->globalPosition().toPoint(), factor);
        event->accept();
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) { close(); return; }
        Preview::mouseDoubleClickEvent(event);
    }
    void showOpacityTag()
    {
        if (!opacityTag) {
            opacityTag = new QLabel(this);
            opacityTag->setObjectName("pinnedSize");
            opacityTag->setAttribute(Qt::WA_TransparentForMouseEvents);
            opacityTag->setStyleSheet(
                "background:rgba(14,14,17,215);border:1px solid rgba(255,255,255,28);"
                "border-radius:8px;color:#C9C9CF;font-size:10px;padding:3px 7px;");
            opacityTimer.setSingleShot(true);
            connect(&opacityTimer, &QTimer::timeout, opacityTag, &QWidget::hide);
        }
        opacityTag->setText(QStringLiteral("%1%").arg(int(windowOpacity() * 100)));
        opacityTag->adjustSize();
        opacityTag->move(width() - opacityTag->width() - 10,
                         height() - opacityTag->height() - 10);
        opacityTag->show();
        opacityTag->raise();
        opacityTimer.start(900);
    }
private:
    qreal zoomScale = 1.0;
    QLabel *opacityTag = nullptr;
    QTimer opacityTimer;
    QWidget *bar = new QWidget(this);
    QPushButton *toggle = nullptr;
    QPushButton *copyButton = nullptr;
    QPushButton *save = nullptr;
    QPushButton *closeButton = nullptr;
    QLabel *sizeTag = nullptr;
    QGraphicsOpacityEffect *barOpacity = nullptr;
    QGraphicsOpacityEffect *sizeOpacity = nullptr;
    QVariantAnimation *chromeFade = nullptr;
    QPropertyAnimation *windowFade = nullptr;
    QElapsedTimer chromeHold;
    bool chromeVisible = false;

    QPushButton *pinButton(const QString &symbol, const QString &tip)
    {
        auto *button = new HoverButton(bar);
        button->setFixedSize(30, 30);
        button->setIconSize({16, 16});
        if (!symbol.isEmpty()) button->setIcon(iconTint(symbol, kIcon, 16));
        button->setToolTip(tip);
        button->setAccessibleName(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    }
    bool isPinned() const { return windowFlags().testFlag(Qt::WindowStaysOnTopHint); }
    void placeChildren()
    {
        bar->move(width() - bar->width() - 8, 8);
        sizeTag->move(10, height() - sizeTag->height() - 10);
    }
    void showChrome() { setChromeVisible(true); }
    void flashToolbar()
    {
        chromeHold.start();
        showChrome();
        QTimer::singleShot(1400, this, [this]{ syncToolbar(); });
    }
    void syncToolbar()
    {
        if (chromeHold.isValid() && chromeHold.elapsed() < 1400) return;
        auto *focus = QApplication::focusWidget();
        const bool reveal = underMouse() || focus == this || (focus && bar->isAncestorOf(focus));
        setChromeVisible(reveal);
    }
    void setChromeVisible(bool visible)
    {
        const qreal target = visible ? 1.0 : 0.0;
        if (chromeVisible == visible && chromeFade->state() == QAbstractAnimation::Running) return;
        if (chromeVisible == visible && std::abs(barOpacity->opacity() - target) < 0.01) {
            if (visible) {
                bar->show();
                sizeTag->show();
                bar->raise();
                sizeTag->raise();
            }
            return;
        }
        chromeVisible = visible;
        chromeFade->stop();
        if (visible) {
            bar->show();
            sizeTag->show();
            bar->raise();
            sizeTag->raise();
        }
        chromeFade->setStartValue(barOpacity->opacity());
        chromeFade->setEndValue(target);
        chromeFade->start();
    }
    void setPinned(bool on)
    {
        if (on != isPinned()) {
            const QRect previous = geometry();
            setWindowFlag(Qt::WindowStaysOnTopHint, on);
            setGeometry(previous);
            show();
        }
        setWindowTitle(on ? "Lumenshot · 置顶预览" : "Lumenshot · 图片预览");
        toggle->setIcon(iconTint(on ? "pin-off" : "pin", kIcon, 16));
        toggle->setToolTip(on ? "取消置顶" : "置顶");
        toggle->setAccessibleName(toggle->toolTip());
    }
};

// 延时截图倒计时：覆盖当前屏幕，任意点击或 Esc 可取消。
class Countdown : public QDialog {
public:
    Countdown(const QRect &area, int seconds, QWidget *parent = nullptr)
        : QDialog(parent), remaining(seconds)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setGeometry(area);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        timer.setInterval(1000);
        connect(&timer, &QTimer::timeout, this, [this]{
            if (--remaining <= 0) accept();
            else update();
        });
        timer.start();
        show();
        raise();
        setFocus();
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(4, 7, 9, 96));
        const QRectF card(QPointF(rect().center()) - QPointF(110, 78), QSizeF(220, 156));
        p.setPen(QPen(QColor(255, 255, 255, 30), 1));
        p.setBrush(QColor(12, 12, 15, 236));
        p.drawRoundedRect(card, 18, 18);
        QFont number = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        number.setPixelSize(64);
        number.setWeight(QFont::DemiBold);
        p.setFont(number);
        p.setPen(kMint);
        p.drawText(card.adjusted(0, 12, 0, -34), Qt::AlignCenter, QString::number(remaining));
        QFont hint = font();
        hint.setPixelSize(11);
        p.setFont(hint);
        p.setPen(QColor("#8E8E97"));
        p.drawText(card.adjusted(0, 0, 0, -14), Qt::AlignHCenter | Qt::AlignBottom,
                   "秒后开始截图 · Esc 取消");
    }
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape) { reject(); return; }
        QDialog::keyPressEvent(event);
    }
    void mousePressEvent(QMouseEvent *) override { reject(); }
private:
    QTimer timer;
    int remaining;
};

// 全局贴图注册表：主面板“贴图管理”弹层读取。
QList<QPointer<PinnedPreview>> &livePins()
{
    static QList<QPointer<PinnedPreview>> pins;
    pins.removeAll(nullptr);
    return pins;
}

// 贴图管理弹层：列出当前所有置顶贴图，支持聚焦、逐个关闭、全部关闭。
class PinPopup : public QWidget {
public:
    PinPopup(QWidget *parent) : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    {
        setObjectName("shotPopup");
        setAttribute(Qt::WA_TranslucentBackground);
        auto *column = new QVBoxLayout(this);
        column->setContentsMargins(12, 11, 12, 12);
        column->setSpacing(8);
        setMinimumSize(300, 170);
        auto *title = new QLabel("置顶贴图");
        column->addWidget(title);
        auto &pins = livePins();
        auto *grid = new QGridLayout;
        grid->setSpacing(7);
        int index = 0;
        for (const QPointer<PinnedPreview> &pin : pins) {
            if (!pin) continue;
            auto *button = new QPushButton;
            button->setObjectName("shotThumb");
            button->setFixedSize(104, 68);
            button->setIconSize({98, 62});
            button->setIcon(QIcon(QPixmap::fromImage(
                pin->masks.image().scaled(98, 62, Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation))));
            button->setToolTip("点击聚焦该贴图");
            button->setCursor(Qt::PointingHandCursor);
            button->setFocusPolicy(Qt::NoFocus);
            connect(button, &QPushButton::clicked, this, [this, pin]{
                if (pin) { pin->showNormal(); pin->raise(); pin->activateWindow(); }
                close();
            });
            auto *close = new QPushButton("关闭");
            close->setFixedHeight(68);
            close->setStyleSheet("QPushButton{background:rgba(255,69,58,36);color:#FFB4AC;"
                                 "border:1px solid rgba(255,69,58,90);border-radius:8px;"
                                 "padding:0 10px;font-size:11px;}"
                                 "QPushButton:hover{background:rgba(255,69,58,72);}");
            close->setCursor(Qt::PointingHandCursor);
            close->setFocusPolicy(Qt::NoFocus);
            connect(close, &QPushButton::clicked, this, [this, pin]{
                if (pin) pin->close();
                QWidget::close();
            });
            auto *cell = new QHBoxLayout;
            cell->setSpacing(6);
            cell->addWidget(button);
            cell->addWidget(close);
            grid->addLayout(cell, index, 0);
            ++index;
        }
        column->addLayout(grid);
        if (index == 0) {
            column->addStretch();
            auto *empty = new QLabel("当前没有贴图\n按 F1 截图后点「置顶悬浮」即可贴图");
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet("color:#8E8E97;font-size:12px;padding:14px 20px;");
            column->addWidget(empty);
            column->addStretch();
        } else {
            auto *closeAll = new QPushButton("全部关闭");
            closeAll->setObjectName("popupMore");
            closeAll->setFixedHeight(26);
            closeAll->setCursor(Qt::PointingHandCursor);
            closeAll->setFocusPolicy(Qt::NoFocus);
            connect(closeAll, &QPushButton::clicked, this, [this]{
                for (const QPointer<PinnedPreview> &pin : livePins())
                    if (pin) pin->close();
                QWidget::close();
            });
            column->addWidget(closeAll);
        }
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(frame, 12, 12);
        painter.fillPath(path, QColor("#141416"));
        painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
};


QIcon annotationIcon(MaskShape shape, const QColor &color, int size)
{
    switch (shape) {
    case MaskShape::Rectangle: return iconTint("rectangle-horizontal", color, size);
    case MaskShape::Ellipse: return iconTint("circle", color, size);
    case MaskShape::Arrow: return iconTint("move-up-right", color, size);
    case MaskShape::Line: return shapeIcon("line", color, size);
    case MaskShape::Highlight: return shapeIcon("highlighter", color, size);
    case MaskShape::Number: return shapeIcon("number", color, size);
    case MaskShape::Text: return iconTint("type", color, size);
    case MaskShape::Blur: return iconTint("blend", color, size);
    case MaskShape::Mosaic: return iconTint("grid-3x3", color, size);
    case MaskShape::Pen: return shapeIcon("pen", color, size);
    case MaskShape::RoundedRect: return shapeIcon("rounded", color, size);
    case MaskShape::Bubble: return shapeIcon("bubble", color, size);
    case MaskShape::MosaicBrush: return shapeIcon("mbrush", color, size);
    case MaskShape::Crop: return shapeIcon("crop", color, size);
    default: return {};
    }
}

// 跟随截图卡片的浮动工具条：最多两行图标，位置由 Selection 负责避让屏幕边缘。
class FloatingToolbar : public QWidget {
    Q_OBJECT
public:
    struct Actions {
        std::function<void()> pointer;
        std::function<void(MaskShape)> tool;
#ifdef LUMENSHOT_OCR
        std::function<void()> ocr;
#endif
        std::function<void()> undo, redo, copy, save, beautify, pin, close;
    };

    FloatingToolbar(Actions handlers, QWidget *parent)
        : QWidget(parent), actions(std::move(handlers))
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet(
            "QPushButton{background:transparent;border:0;border-radius:7px;padding:0;}"
            "QPushButton:hover{background:transparent;}"
            "QPushButton:pressed{background:rgba(255,255,255,12);}"
            "QPushButton:checked{background:rgba(163,240,206,40);}"
            "QPushButton:disabled{background:transparent;}");
        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(18);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 150));
        setGraphicsEffect(shadow);

        auto *grid = new QGridLayout(this);
        grid->setContentsMargins(5, 5, 5, 5);
        grid->setSpacing(1);

        auto group = new QButtonGroup(this);
        group->setExclusive(true);
        // 12 列两行：第一行绘制形状，第二行特效与操作
        const auto place = [grid](QWidget *button, int index) {
            grid->addWidget(button, index / 12, index % 12);
        };

        pointerButton = button(shapeIcon("pointer", kIcon, 16),
                               shapeIcon("pointer", kMint, 16), "移动卡片 · 拖动图片", true);
        group->addButton(pointerButton);
        connect(pointerButton, &QPushButton::clicked, this, [this]{
            if (actions.pointer) actions.pointer();
        });
        place(pointerButton, 0);

        const QList<QPair<MaskShape, QString>> entries = {
            {MaskShape::Rectangle, "矩形遮挡 · 1"}, {MaskShape::Ellipse, "椭圆遮挡 · 2"},
            {MaskShape::RoundedRect, "圆角框 · R"}, {MaskShape::Bubble, "气泡 · W"},
            {MaskShape::Arrow, "箭头 · 3"}, {MaskShape::Line, "直线 · 7"},
            {MaskShape::Highlight, "高亮笔 · 8"}, {MaskShape::Pen, "画笔 · B"},
            {MaskShape::Number, "序号标记 · 9"}, {MaskShape::Text, "文字 · 4"},
            {MaskShape::Crop, "裁剪 · 0"}, {MaskShape::Blur, "模糊 · 5"},
            {MaskShape::Mosaic, "马赛克 · 6"}, {MaskShape::MosaicBrush, "马赛克笔刷 · M"},
        };
        int index = 1;
        for (const auto &[shape, tip] : entries) {
            auto *item = button(annotationIcon(shape, kIcon, 16),
                                annotationIcon(shape, kMint, 16), tip, true);
            group->addButton(item);
            connect(item, &QPushButton::clicked, this, [this, shape]{
                if (actions.tool) actions.tool(shape);
            });
            toolButtons.append({shape, item});
            place(item, index++);
        }

#ifdef LUMENSHOT_OCR
        auto *ocrButton = button(shapeIcon("scan-text", kIcon, 16),
                                 shapeIcon("scan-text", kMint, 16), "提取文字 · O", false);
        connect(ocrButton, &QPushButton::clicked, this, [this]{
            if (actions.ocr) actions.ocr();
        });
        place(ocrButton, index++);
#endif

        undoButton = button(iconTint("undo-2", kIcon, 16), iconTint("undo-2", kMint, 16),
                            "撤销 · Ctrl+Z", false);
        connect(undoButton, &QPushButton::clicked, this, [this]{ if (actions.undo) actions.undo(); });
        place(undoButton, index++);
        redoButton = button(iconTint("redo-2", kIcon, 16), iconTint("redo-2", kMint, 16),
                            "重做 · Ctrl+Y", false);
        connect(redoButton, &QPushButton::clicked, this, [this]{ if (actions.redo) actions.redo(); });
        place(redoButton, index++);

        struct Extra { QIcon icon; QIcon active; QString tip; std::function<void()> action; };
        const QList<Extra> extras = {
            {iconTint("copy", kIcon, 16), iconTint("copy", kMint, 16), "复制 · Ctrl+C", actions.copy},
            {iconTint("download", kIcon, 16), iconTint("download", kMint, 16), "保存 · Ctrl+S", actions.save},
            {shapeIcon("sparkles", kIcon, 16), shapeIcon("sparkles", kMint, 16), "美化保存", actions.beautify},
            {iconTint("pin", kIcon, 16), iconTint("pin", kMint, 16), "置顶悬浮", actions.pin},
            {iconTint("x", kIcon, 16), iconTint("x", kMint, 16), "关闭 · Esc", actions.close},
        };
        for (const auto &extra : extras) {
            auto *item = button(extra.icon, extra.active, extra.tip, false);
            connect(item, &QPushButton::clicked, this, [action = extra.action]{
                if (action) action();
            });
            place(item, index++);
        }
        undoButton->setEnabled(false);
        redoButton->setEnabled(false);
        resize(sizeHint());
    }

    void setActive(MaskShape shape, bool pointer)
    {
        pointerButton->setChecked(pointer);
        for (const auto &[itemShape, button] : toolButtons)
            button->setChecked(!pointer && itemShape == shape);
    }
    void setHistory(bool canUndo, bool canRedo)
    {
        undoButton->setEnabled(canUndo);
        redoButton->setEnabled(canRedo);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QLinearGradient fill(0, body.top(), 0, body.bottom());
        fill.setColorAt(0.0, QColor(28, 28, 33, 244));
        fill.setColorAt(1.0, QColor(17, 17, 20, 244));
        p.setPen(QPen(QColor(255, 255, 255, 34), 1));
        p.setBrush(fill);
        p.drawRoundedRect(body, 11, 11);
    }

private:
    Actions actions;
    QPushButton *pointerButton = nullptr;
    QPushButton *undoButton = nullptr;
    QPushButton *redoButton = nullptr;
    QList<QPair<MaskShape, QPushButton *>> toolButtons;

    QPushButton *button(const QIcon &normal, const QIcon &active,
                        const QString &tip, bool checkable)
    {
        auto *item = new HoverButton(this);
        item->setFixedSize(28, 28);
        item->setIconSize({16, 16});
        item->setIcon(normal);
        item->setCheckable(checkable);
        item->setToolTip(tip);
        item->setAccessibleName(tip);
        item->setCursor(Qt::PointingHandCursor);
        item->setFocusPolicy(Qt::NoFocus);
        item->setStateIcons(normal, active);
        return item;
    }
};

class Selection : public QDialog {
public:
    Selection(const QImage &frame, QScreen *screen, bool pickWindow, QList<WindowFrame> frames,
              const QRect &preselected = {})
        : frame(frame), pickWindow(pickWindow), frames(std::move(frames)), origin(screen->geometry().topLeft())
    {
        color = QSettings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop")
                    .value("annotation/color", defaultColor).value<QColor>();
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setScreen(screen);
        setGeometry(screen->geometry());
        setCursor(pickWindow ? Qt::PointingHandCursor : Qt::CrossCursor);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setWindowState(Qt::WindowFullScreen);
        setFocus();
        if (!preselected.isEmpty())
            QTimer::singleShot(0, this, [this, preselected]{ beginCard(preselected); });
    }
    QString failure;
    std::function<void(const QImage &)> onCaptured;
    const QColor defaultColor = QColor("#FF5F57");
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawImage(rect(), frame);
        if (cardMode) {
            p.fillRect(rect(), QColor(4, 7, 9, 142));
            return;
        }

        const QRectF focus = pickWindow ? QRectF(hovered) : selected;
        p.save();
        if (!focus.isEmpty())
            p.setClipRegion(QRegion(rect()).subtracted(focus.toAlignedRect()));
        p.fillRect(rect(), QColor(6, 9, 11, pickWindow ? 118 : 106));
        p.restore();

        if (pickWindow) {
            if (!hovered.isEmpty()) {
                paintFocus(p, QRectF(hovered), false);
                paintSize(p, QRectF(hovered));
            }
        } else {
            if (hasCursor && !dragging) {
                p.setPen(QPen(QColor(255, 255, 255, 42), 1));
                p.drawLine(QPointF(cursor.x(), 0), QPointF(cursor.x(), height()));
                p.drawLine(QPointF(0, cursor.y()), QPointF(width(), cursor.y()));
            }
            if (!selected.isEmpty()) {
                paintFocus(p, selected, true);
                paintSize(p, selected);
            }
            paintLoupe(p);
        }
        paintHint(p);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (cardMode) {
            if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
                finish();
            return;
        }
        if (event->button() == Qt::RightButton) { reject(); return; }
        if (event->button() != Qt::LeftButton) return;
        if (!pickWindow && loupeColor.isValid() && loupeCardRect.contains(event->position())) {
            QApplication::clipboard()->setText(loupeColor.name().toUpper());
            loupeCopied = true;
            update();
            QTimer::singleShot(900, this, [this]{ loupeCopied = false; update(); });
            return;
        }
        if (pickWindow) {
            QString message;
            const QRect target = windowAt(event->position(), &message);
            if (target.isEmpty()) {
                failure = message;
                reject();
            } else {
                beginCard(target);
            }
            return;
        }
        start = event->position();
        before = selected;
        edges = hitEdges(start);
        moving = !edges && selected.contains(start);
        if (!moving && !edges) selected = {};
        dragging = true;
        update();
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (cardMode) return;
        if (pickWindow) {
            const QRect next = windowAt(event->position());
            if (next != hovered) { hovered = next; update(); }
            return;
        }
        cursor = event->position();
        hasCursor = true;
        if (dragging) {
            adjust(cursor);
        } else {
            auto hit = hitEdges(cursor);
            const bool horizontal = hit & (Qt::LeftEdge | Qt::RightEdge);
            const bool vertical = hit & (Qt::TopEdge | Qt::BottomEdge);
            if (horizontal && vertical)
                setCursor(hit.testFlag(Qt::LeftEdge) == hit.testFlag(Qt::TopEdge) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
            else if (horizontal || vertical) setCursor(horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor);
            else setCursor(selected.contains(cursor) ? Qt::SizeAllCursor : Qt::CrossCursor);
        }
        update();
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (cardMode) return;
        if (!dragging || event->button() != Qt::LeftButton) return;
        adjust(event->position());
        dragging = false;
        if (!selected.isEmpty())
            beginCard(pixelRect(selected));
    }
    void leaveEvent(QEvent *event) override
    {
        hasCursor = false;
        update();
        QDialog::leaveEvent(event);
    }
    void keyPressEvent(QKeyEvent *event) override
    {
        if (cardMode) {
            if (event->key() == Qt::Key_Escape) {
#ifdef LUMENSHOT_OCR
                if (ocrPanel) { closeOcrPanel(); return; }
#endif
                if (cardView && cardView->cropMode) {
                    cardView->cropMode = false;
                    activatePointer();
                    return;
                }
                if (cardView && cardView->editing) activatePointer();
                else finish();
                return;
            }
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                if (cardView && cardView->cropMode) {
                    cardView->applyCrop();
                    return;
                }
                copyCard();
                return;
            }
            return;
        }
        if (event->key() == Qt::Key_Escape) { reject(); return; }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (pickWindow) {
                if (!hovered.isEmpty()) beginCard(hovered);
            } else if (!selected.isEmpty()) {
                beginCard(pixelRect(selected));
            }
            return;
        }
        QDialog::keyPressEvent(event);
    }
private:
    const QImage &frame;
    bool pickWindow;
    QList<WindowFrame> frames;
    QPoint origin;
    QPointF start;
    QPointF cursor;
    QRectF selected, before;
    QRect hovered;
    QRectF loupeCardRect;
    QColor loupeColor;
    bool loupeCopied = false;
    Qt::Edges edges;
    bool moving = false;
    bool dragging = false;
    bool hasCursor = false;
    bool cardMode = false;
    Preview *cardView = nullptr;
    FloatingToolbar *toolbar = nullptr;
    ColorStrip *colorStrip = nullptr;
    QColor color = Qt::black;
    QList<QColor> recentColors;
#ifdef LUMENSHOT_OCR
    Ocr ocr;
    bool ocrRunning = false;
    OcrResultPanel *ocrPanel = nullptr;
    QList<QShortcut *> cardShortcuts;
#endif

    QRect pixelRect(const QRectF &area) const
    {
        if (area.isEmpty() || frame.isNull() || width() <= 0 || height() <= 0) return {};
        const auto x = [&](double value) { return (value - rect().x()) * frame.width() / width(); };
        const auto y = [&](double value) { return (value - rect().y()) * frame.height() / height(); };
        const int left = qFloor(x(area.left()));
        const int top = qFloor(y(area.top()));
        const int right = qCeil(x(area.right()));
        const int bottom = qCeil(y(area.bottom()));
        return QRect(left, top, right - left, bottom - top).intersected(frame.rect());
    }

    QRect toDialog(const QRect &pixels) const
    {
        const qreal sx = width() / qreal(frame.width());
        const qreal sy = height() / qreal(frame.height());
        return QRect(qRound(pixels.x() * sx), qRound(pixels.y() * sy),
                     qRound(pixels.width() * sx), qRound(pixels.height() * sy));
    }

    void beginCard(const QRect &pixelsInFrame)
    {
        if (cardMode || frame.isNull()) return;
        const QRect area = pixelsInFrame.normalized().intersected(frame.rect());
        if (area.width() < 2 || area.height() < 2) return;
        const QImage shot = frame.copy(area);
        if (shot.isNull()) return;

        cardMode = true;
        const QRect card = toDialog(area);
        if (onCaptured) onCaptured(shot);

        cardView = new Preview(this);
        cardView->floating = true;
        cardView->cardDraggable = true;
        cardView->selectable = true;
        cardView->color = color;
        cardView->setMinimumSize(0, 0);
        cardView->setAttribute(Qt::WA_NoMousePropagation);
        cardView->masks.reset(shot);
        cardView->setCursor(Qt::OpenHandCursor);
        cardView->setGeometry(card.adjusted(-Preview::shadowPad, -Preview::shadowPad,
                                            Preview::shadowPad, Preview::shadowPad));
        cardView->show();
        connect(cardView, &Preview::edited, this, [this]{ refreshToolbar(); });
        connect(cardView, &Preview::moved, this, [this]{ placeToolbar(); });
        connect(cardView, &Preview::pressed, this, [this]{
            if (colorStrip) colorStrip->hide();
#ifdef LUMENSHOT_OCR
            closeOcrPanel();
#endif
        });
        // 选中已有标注时自动带出颜色条，弥补工具栏色板按钮的移除。
        connect(cardView, &Preview::picked, this, [this](int index){
            if (!colorStrip) return;
            if (index >= 0) {
                colorStrip->setCurrent(color);
                colorStrip->show();
                placeColorStrip();
            } else {
                colorStrip->hide();
            }
        });
        connect(cardView, &Preview::cropped, this, [this](const QRect &pixels){
            if (!cardView) return;
            const QPoint oldImage = cardView->pos() + QPoint(Preview::shadowPad, Preview::shadowPad);
            const QPoint newImage = oldImage + pixels.topLeft();
            const QSize size = cardView->masks.image().size();
            cardView->setGeometry(QRect(newImage - QPoint(Preview::shadowPad, Preview::shadowPad),
                                        size + QSize(Preview::shadowPad * 2, Preview::shadowPad * 2)));
            refreshToolbar();
        });

        FloatingToolbar::Actions actions;
        actions.pointer = [this]{ activatePointer(); };
        actions.tool = [this](MaskShape shape){ setTool(shape); };
#ifdef LUMENSHOT_OCR
        actions.ocr = [this]{ runOcr(); };
#endif
        actions.undo = [this]{ undoCard(); };
        actions.redo = [this]{ redoCard(); };
        actions.copy = [this]{ copyCard(); };
        actions.save = [this]{ saveCard(); };
        actions.beautify = [this]{ beautifyCard(); };
        actions.pin = [this]{ pinCard(); };
        actions.close = [this]{ finish(); };
        toolbar = new FloatingToolbar(std::move(actions), this);
        toolbar->setAttribute(Qt::WA_NoMousePropagation);
        toolbar->setActive(MaskShape::Rectangle, true);
        placeToolbar();
        toolbar->show();
        toolbar->raise();

        colorStrip = new ColorStrip(
            [this](const QColor &value){ applyColor(value); },
            [this]{ openCustomColor(); },
            [this](int value){ applyStyleSize(value); },
            [this](int value){ applyStyleAlpha(value); },
            [this]{ placeColorStrip(); },
            this);
        colorStrip->setAttribute(Qt::WA_NoMousePropagation);
        colorStrip->setCurrent(color);
        colorStrip->hide();

        auto bind = [this](const QKeySequence &keys, std::function<void()> action) {
            auto *shortcut = new QShortcut(keys, this);
            shortcut->setContext(Qt::WindowShortcut);
            connect(shortcut, &QShortcut::activated, this, action);
#ifdef LUMENSHOT_OCR
            cardShortcuts.append(shortcut);
#endif
        };
        bind(QKeySequence("1"), [this]{ setTool(MaskShape::Rectangle); });
        bind(QKeySequence("2"), [this]{ setTool(MaskShape::Ellipse); });
        bind(QKeySequence("3"), [this]{ setTool(MaskShape::Arrow); });
        bind(QKeySequence("4"), [this]{ setTool(MaskShape::Text); });
        bind(QKeySequence("5"), [this]{ setTool(MaskShape::Blur); });
        bind(QKeySequence("6"), [this]{ setTool(MaskShape::Mosaic); });
        bind(QKeySequence("7"), [this]{ setTool(MaskShape::Line); });
        bind(QKeySequence("8"), [this]{ setTool(MaskShape::Highlight); });
        bind(QKeySequence("9"), [this]{ setTool(MaskShape::Number); });
        bind(QKeySequence("0"), [this]{ setTool(MaskShape::Crop); });
#ifdef LUMENSHOT_OCR
        bind(QKeySequence("O"), [this]{ runOcr(); });
#endif
        bind(QKeySequence("B"), [this]{ setTool(MaskShape::Pen); });
        bind(QKeySequence("R"), [this]{ setTool(MaskShape::RoundedRect); });
        bind(QKeySequence("W"), [this]{ setTool(MaskShape::Bubble); });
        bind(QKeySequence("M"), [this]{ setTool(MaskShape::MosaicBrush); });
        bind(QKeySequence::Undo, [this]{ undoCard(); });
        bind(QKeySequence("Ctrl+Y"), [this]{ redoCard(); });
        bind(QKeySequence("Ctrl+Shift+Z"), [this]{ redoCard(); });
        bind(QKeySequence::Save, [this]{ saveCard(); });
        bind(QKeySequence::Copy, [this]{ copyCard(); });
        bind(QKeySequence::Delete, [this]{ deleteSelected(); });

        setCursor(Qt::ArrowCursor);
        setFocus();
        update();
    }

    void placeToolbar()
    {
        if (!toolbar || !cardView) return;
        toolbar->adjustSize();
        const QSize size = toolbar->size();
        const QRect card = cardView->geometry().adjusted(Preview::shadowPad, Preview::shadowPad,
                                                         -Preview::shadowPad, -Preview::shadowPad);
        const int gap = 10, margin = 8;
        int x = card.right() - size.width() + 1;
        int y = card.top() - size.height() - gap;
        if (y < margin) {
            y = card.bottom() + gap + 1;
            if (y + size.height() > height() - margin) {
                x = card.right() - size.width() - margin + 1;
                y = card.top() + margin;
            }
        }
        x = std::clamp(x, margin, std::max(margin, width() - size.width() - margin));
        y = std::clamp(y, margin, std::max(margin, height() - size.height() - margin));
        toolbar->move(x, y);
        toolbar->raise();
        placeColorStrip();
    }

    void placeColorStrip()
    {
        if (!colorStrip || !colorStrip->isVisible() || !toolbar) return;
        colorStrip->adjustSize();
        const QSize size = colorStrip->size();
        const int gap = 6, margin = 8;
        int x = toolbar->x() + toolbar->width() - size.width();
        int y = toolbar->y() - size.height() - gap;
        if (y < margin) y = toolbar->y() + toolbar->height() + gap;
        x = std::clamp(x, margin, std::max(margin, width() - size.width() - margin));
        y = std::clamp(y, margin, std::max(margin, height() - size.height() - margin));
        colorStrip->move(x, y);
        colorStrip->raise();
    }

    void setTool(MaskShape shape)
    {
        if (!cardView) return;
        cardView->shape = shape;
        cardView->editing = true;
        cardView->selectedStep = -1;
        cardView->setCursor(Qt::CrossCursor);
        switch (shape) {
        case MaskShape::Arrow:
        case MaskShape::Line:
        case MaskShape::Pen: cardView->toolSize = 4; break;
        case MaskShape::Highlight: cardView->toolSize = 18; break;
        case MaskShape::Number: cardView->toolSize = 24; break;
        case MaskShape::Text: cardView->toolSize = 28; break;
        case MaskShape::Blur:
        case MaskShape::Mosaic: cardView->toolSize = 12; break;
        case MaskShape::MosaicBrush: cardView->toolSize = 26; break;
        default: break;
        }
        cardView->alphaPercent =
            shape == MaskShape::RoundedRect ? 75 : 100;
        if (shape == MaskShape::Crop) cardView->beginCrop();
        toolbar->setActive(shape, false);
        if (colorStrip) {
            if (shape == MaskShape::Blur || shape == MaskShape::Mosaic ||
                shape == MaskShape::MosaicBrush || shape == MaskShape::Crop) {
                colorStrip->hide();
            } else {
                colorStrip->setCurrent(color);
                colorStrip->setStyleValues(cardView->toolSize, cardView->alphaPercent);
                colorStrip->show();
                placeColorStrip();
            }
        }
        cardView->update();
        setFocus();
    }

    void activatePointer()
    {
        if (!cardView) return;
        cardView->editing = false;
        cardView->cropMode = false;
        cardView->setCursor(cardView->selectedStep >= 0 ? Qt::SizeAllCursor : Qt::OpenHandCursor);
        toolbar->setActive(cardView->shape, true);
        if (colorStrip) colorStrip->hide();
        cardView->update();
        setFocus();
    }

    void refreshToolbar()
    {
        if (!cardView || !toolbar) return;
        toolbar->setHistory(cardView->masks.canUndo(), cardView->masks.canRedo());
        toolbar->setActive(cardView->shape, !cardView->editing);
        placeToolbar();
    }

    void deleteSelected()
    {
        if (!cardView) return;
        const int index = cardView->selectedStep;
        if (index < 0 || index >= cardView->masks.stepCount()) return;
        cardView->selectedStep = -1;
        cardView->edit([index](Masks &model){ model.removeStep(index); },
                       cardView->masks.isLarge());
    }

    void undoCard()
    {
        if (!cardView || !cardView->masks.canUndo()) return;
        cardView->selectedStep = -1;
        cardView->edit([](Masks &model){ model.undo(); }, cardView->masks.isLarge());
    }

    void redoCard()
    {
        if (!cardView || !cardView->masks.canRedo()) return;
        cardView->edit([](Masks &model){ model.redo(); }, cardView->masks.isLarge());
    }

    void copyCard()
    {
        if (!cardView) return;
        QApplication::clipboard()->setImage(cardView->masks.image());
        if (toolbar)
            QToolTip::showText(toolbar->mapToGlobal(QPoint(toolbar->width() / 2,
                                                            toolbar->height() + 4)),
                               "已复制到剪贴板", toolbar);
    }

#ifdef LUMENSHOT_OCR
    static QString ocrModelDir()
    {
        if (const QString custom = qEnvironmentVariable("LUMENSHOT_OCR_DIR"); !custom.isEmpty())
            return custom;
        const QString installed = QCoreApplication::applicationDirPath() + "/../share/ocr";
        if (QFileInfo::exists(installed + "/ch_PP-OCRv4_det_infer.onnx")) return installed;
        return QStringLiteral(LUMENSHOT_OCR_ASSETS);
    }

    void runOcr()
    {
        if (!cardView || ocrRunning) return;
        const QImage image = cardView->masks.image();
        if (image.isNull()) return;
        ocrRunning = true;
        auto *progress = new QProgressDialog(QStringLiteral("正在识别文字…"), QString(), 0, 0, this);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(150);
        progress->setValue(0);
        auto *watcher = new QFutureWatcher<QList<OcrLine>>(this);
        connect(watcher, &QFutureWatcher<QList<OcrLine>>::finished, this,
                [this, watcher, progress]{
            progress->deleteLater();
            const QList<OcrLine> lines = watcher->result();
            watcher->deleteLater();
            ocrRunning = false;
            if (lines.isEmpty()) {
                QToolTip::showText(QCursor::pos(),
                                   ocr.isLoaded() ? QStringLiteral("未识别到文字")
                                                  : ocr.lastError(),
                                   this);
                return;
            }
            QString text;
            for (const OcrLine &line : lines) text += line.text + QLatin1Char('\n');
            text.chop(1);
            openOcrPanel(text);
        });
        watcher->setFuture(QtConcurrent::run([this, image]{
            if (!ocr.isLoaded()) ocr.ensureLoaded(ocrModelDir());
            return ocr.run(image);
        }));
    }

    // OCR 面板打开期间禁用卡片快捷键，避免在文本框里打字误触工具。
    void setCardShortcuts(bool enabled)
    {
        for (QShortcut *shortcut : cardShortcuts) shortcut->setEnabled(enabled);
    }
    void openOcrPanel(const QString &text)
    {
        if (ocrPanel) { ocrPanel->deleteLater(); ocrPanel = nullptr; }
        ocrPanel = new OcrResultPanel(text, this);
        connect(ocrPanel, &OcrResultPanel::closed, this, [this]{
            if (ocrPanel) { ocrPanel->deleteLater(); ocrPanel = nullptr; }
            setCardShortcuts(true);
        });
        ocrPanel->popup(cardView->geometry().adjusted(Preview::shadowPad, Preview::shadowPad,
                                                      -Preview::shadowPad, -Preview::shadowPad));
        setCardShortcuts(false);
    }
    void closeOcrPanel()
    {
        if (!ocrPanel) return;
        ocrPanel->deleteLater();
        ocrPanel = nullptr;
        setCardShortcuts(true);
    }
#endif

    void saveCard()
    {
        if (cardView && saveImage(this, cardView->masks.image())) finish();
    }

    void beautifyCard()
    {
        if (cardView && saveImage(this, decoratedImage(cardView->masks.image()))) finish();
    }

    void pinCard()
    {
        if (!cardView) return;
        auto *pinned = new PinnedPreview;
        pinned->present(cardView->masks.image());
        const QRect card = cardView->geometry().adjusted(Preview::shadowPad, Preview::shadowPad,
                                                         -Preview::shadowPad, -Preview::shadowPad);
        pinned->move(mapToGlobal(card.topLeft()));
        pinned->show();
        pinned->raise();
        finish();
    }

    void applyColor(const QColor &value)
    {
        if (!value.isValid() || !cardView) return;
        color = value;
        QSettings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop")
            .setValue("annotation/color", value.name(QColor::HexRgb));
        recentColors.removeAll(value);
        recentColors.prepend(value);
        while (recentColors.size() > 6) recentColors.removeLast();
        cardView->color = value;
        if (colorStrip) colorStrip->setCurrent(value);
        if (cardView->selectedStep >= 0 && cardView->selectedStep < cardView->masks.stepCount()) {
            cardView->masks.setStepColor(cardView->selectedStep, value);
            cardView->update();
        }
    }

    void applyStyleSize(int value)
    {
        if (!cardView) return;
        cardView->toolSize = value;
        const int index = cardView->selectedStep;
        if (index >= 0 && index < cardView->masks.stepCount()) {
            cardView->masks.setStepSize(index, value);
            cardView->update();
        }
    }
    void applyStyleAlpha(int value)
    {
        if (!cardView) return;
        cardView->alphaPercent = value;
        const int index = cardView->selectedStep;
        if (index >= 0 && index < cardView->masks.stepCount()) {
            cardView->masks.setStepAlpha(index, value);
            cardView->update();
        }
    }

    void openCustomColor()
    {
        QColorDialog dialog(color, this);
        dialog.setWindowTitle("选择标注颜色");
        dialog.setOption(QColorDialog::DontUseNativeDialog);
        if (dialog.exec() == QDialog::Accepted) applyColor(dialog.currentColor());
    }

    void finish()
    {
        if (!cardMode) return;
#ifdef LUMENSHOT_OCR
        if (ocrPanel) { ocrPanel->deleteLater(); ocrPanel = nullptr; }
#endif
        if (colorStrip) { colorStrip->hide(); colorStrip->deleteLater(); colorStrip = nullptr; }
        if (cardView) { cardView->hide(); cardView->deleteLater(); cardView = nullptr; }
        if (toolbar) { toolbar->hide(); toolbar->deleteLater(); toolbar = nullptr; }
        cardMode = false;
        reject();
    }

    QRect windowAt(const QPointF &position, QString *message = nullptr) const
    {
        const QPoint pixel(qFloor(position.x() * frame.width() / width()),
                           qFloor(position.y() * frame.height() / height()));
        QRegion covered;
        for (const auto &item : frames) {
            const QRect local = item.bounds.translated(-origin);
            if (local.contains(pixel)) {
                if (!item.selectable) break;
                if (covered.intersects(local) || !frame.rect().contains(local)) {
                    if (message) *message = "请将目标窗口完整移到当前屏幕最前方，再重新截图。";
                    return {};
                }
                return local;
            }
            covered += local;
        }
        if (message) *message = "此处没有可识别的应用窗口。";
        return {};
    }

    QSize pixelSize(const QRectF &area) const
    {
        return {qCeil(area.right() * frame.width() / width()) - qFloor(area.left() * frame.width() / width()),
                qCeil(area.bottom() * frame.height() / height()) - qFloor(area.top() * frame.height() / height())};
    }

    void paintFocus(QPainter &p, const QRectF &area, bool handles)
    {
        for (int i = 6; i >= 1; --i) {
            p.setPen(QPen(QColor(163, 240, 206, 4 + (6 - i) * 2), 1));
            p.setBrush(Qt::NoBrush);
            p.drawRect(area.adjusted(-i, -i, i, i));
        }
        p.setPen(QPen(QColor("#B9F7DB"), 1.8));
        p.setBrush(Qt::NoBrush);
        p.drawRect(area.adjusted(0.5, 0.5, -0.5, -0.5));
        if (!handles) return;
        const QList<QPointF> points = {
            area.topLeft(), area.topRight(), area.bottomLeft(), area.bottomRight(),
            QPointF(area.center().x(), area.top()), QPointF(area.center().x(), area.bottom()),
            QPointF(area.left(), area.center().y()), QPointF(area.right(), area.center().y())};
        for (const QPointF &point : points) {
            p.setPen(QPen(QColor(8, 10, 12, 210), 1.4));
            p.setBrush(QColor("#A3F0CE"));
            p.drawEllipse(point, 4.2, 4.2);
        }
    }

    void paintSize(QPainter &p, const QRectF &area)
    {
        const QSize pixels = pixelSize(area);
        const QString text = QString("%1 × %2 px").arg(pixels.width()).arg(pixels.height());
        QFont badgeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        badgeFont.setPixelSize(11);
        badgeFont.setWeight(QFont::Medium);
        const QFontMetrics metrics(badgeFont);
        const qreal width = metrics.horizontalAdvance(text) + 18;
        const qreal height = 23;
        qreal x = area.left();
        qreal y = area.top() - height - 8;
        if (y < 8) y = area.top() + 8;
        x = qBound(8.0, x, this->width() - width - 8.0);
        y = qBound(8.0, y, this->height() - height - 8.0);
        const QRectF badge(x, y, width, height);
        p.setPen(QPen(QColor(255, 255, 255, 32), 1));
        p.setBrush(QColor(10, 10, 12, 228));
        p.drawRoundedRect(badge, 7, 7);
        p.setFont(badgeFont);
        p.setPen(QColor("#E9E9EC"));
        p.drawText(badge, Qt::AlignCenter, text);
    }

    void paintLoupe(QPainter &p)
    {
        loupeCardRect = {};
        if (pickWindow || dragging || !hasCursor || frame.isNull()) return;
        if (!selected.isEmpty() && !hitEdges(cursor)) return;
        const qreal scaleX = frame.width() / qreal(width());
        const qreal scaleY = frame.height() / qreal(height());
        const QPoint pixel(qRound(cursor.x() * scaleX), qRound(cursor.y() * scaleY));
        if (!frame.rect().contains(pixel)) return;

        const int zoomWidth = 126, zoomHeight = 91, footer = 21, pad = 1;
        QRect source(0, 0, 18, 13);
        source.moveCenter(pixel);
        if (frame.width() >= source.width())
            source.moveLeft(std::clamp(source.left(), 0, frame.width() - source.width()));
        else
            source.setWidth(frame.width());
        if (frame.height() >= source.height())
            source.moveTop(std::clamp(source.top(), 0, frame.height() - source.height()));
        else
            source.setHeight(frame.height());
        source = source.intersected(frame.rect());
        if (source.isEmpty()) return;

        const qreal cardWidth = zoomWidth + pad * 2;
        const qreal cardHeight = zoomHeight + footer + pad * 2;
        qreal x = cursor.x() + 24;
        qreal y = cursor.y() + 24;
        if (x + cardWidth > width() - 12) x = cursor.x() - cardWidth - 24;
        if (y + cardHeight > height() - 76) y = cursor.y() - cardHeight - 24;
        x = std::clamp(x, 12.0, std::max(12.0, width() - cardWidth - 12));
        y = std::clamp(y, 58.0, std::max(58.0, height() - cardHeight - 76));
        const QRectF card(x, y, cardWidth, cardHeight);

        QPainterPath clip;
        clip.addRoundedRect(card, 10, 10);
        p.save();
        p.setClipPath(clip);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        const QRectF imageRect(card.left() + pad, card.top() + pad, zoomWidth, zoomHeight);
        p.drawImage(imageRect, frame, source);
        p.fillRect(QRectF(card.left() + pad, imageRect.bottom() + 1, zoomWidth, footer),
                   QColor(10, 10, 12, 236));
        p.setPen(QPen(QColor(255, 255, 255, 18), 1));
        p.drawLine(QPointF(imageRect.left(), imageRect.bottom() + 1),
                   QPointF(imageRect.right(), imageRect.bottom() + 1));

        const QPointF focus(imageRect.left() + (cursor.x() * scaleX - source.left()) * imageRect.width() / source.width(),
                           imageRect.top() + (cursor.y() * scaleY - source.top()) * imageRect.height() / source.height());
        p.setPen(QPen(QColor(255, 255, 255, 64), 1));
        p.drawLine(QPointF(imageRect.left(), focus.y()), QPointF(imageRect.right(), focus.y()));
        p.drawLine(QPointF(focus.x(), imageRect.top()), QPointF(focus.x(), imageRect.bottom()));
        p.setPen(QPen(QColor(163, 240, 206, 220), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(focus.x() - 5, focus.y() - 5, 10, 10));

        QFont meta = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        meta.setPixelSize(9);
        p.setFont(meta);
        const QRectF bar(card.left() + pad, imageRect.bottom() + 1, zoomWidth, footer);
        loupeColor = frame.pixelColor(pixel);
        loupeCardRect = card;
        p.setPen(Qt::NoPen);
        p.setBrush(loupeColor);
        p.drawRoundedRect(QRectF(bar.left() + 10, bar.center().y() - 3.5, 7, 7), 2, 2);
        p.setPen(QColor("#B9B9C1"));
        p.drawText(QRectF(bar.left() + 21, bar.top(), 52, bar.height()),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1, %2").arg(pixel.x()).arg(pixel.y()));
        p.setPen(loupeCopied ? kMint : QColor("#8E8E97"));
        p.drawText(QRectF(bar.right() - 58, bar.top(), 48, bar.height()),
                   Qt::AlignRight | Qt::AlignVCenter,
                   loupeCopied ? QStringLiteral("已复制") : loupeColor.name().toUpper());
        p.restore();

        p.setPen(QPen(QColor(255, 255, 255, 46), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(card.adjusted(-0.5, -0.5, 0.5, 0.5), 10.5, 10.5);
        p.setPen(QPen(QColor(8, 10, 12, 170), 1));
        p.drawRoundedRect(card, 10, 10);
    }

    void paintHint(QPainter &p)
    {
        struct Piece { QString text; bool chip; };
        const QList<Piece> pieces = pickWindow
            ? QList<Piece>{{"点击选择窗口", false}, {"Esc", true}, {"取消", false}}
            : QList<Piece>{{"拖动选择区域", false}, {"松手立即完成", false},
                           {"Esc", true}, {"取消", false}};
        QFont textFont = font();
        textFont.setPixelSize(11);
        QFont chipFont = font();
        chipFont.setPixelSize(10);
        const QFontMetrics textMetrics(textFont), chipMetrics(chipFont);
        qreal content = 0;
        for (const auto &piece : pieces) {
            content += piece.chip ? chipMetrics.horizontalAdvance(piece.text) + 12
                                  : textMetrics.horizontalAdvance(piece.text);
        }
        content += 8 * (pieces.size() - 1);
        const qreal pillWidth = content + 24, pillHeight = 32;
        const QRectF pill((width() - pillWidth) / 2.0, 16, pillWidth, pillHeight);
        p.setPen(QPen(QColor(255, 255, 255, 28), 1));
        p.setBrush(QColor(14, 14, 17, 214));
        p.drawRoundedRect(pill, 16, 16);
        qreal x = pill.left() + 12;
        for (int i = 0; i < pieces.size(); ++i) {
            const Piece &piece = pieces.at(i);
            if (piece.chip) {
                const qreal chipWidth = chipMetrics.horizontalAdvance(piece.text) + 12;
                const QRectF chip(x, pill.center().y() - 9, chipWidth, 18);
                p.setPen(QPen(QColor(255, 255, 255, 30), 1));
                p.setBrush(QColor(255, 255, 255, 14));
                p.drawRoundedRect(chip, 4.5, 4.5);
                p.setFont(chipFont);
                p.setPen(QColor("#E9E9EC"));
                p.drawText(chip, Qt::AlignCenter, piece.text);
                x += chipWidth;
            } else {
                p.setFont(textFont);
                p.setPen(QColor("#B5B5BD"));
                p.drawText(QRectF(x, pill.center().y() - 11, textMetrics.horizontalAdvance(piece.text), 22),
                           Qt::AlignLeft | Qt::AlignVCenter, piece.text);
                x += textMetrics.horizontalAdvance(piece.text);
            }
            if (i + 1 < pieces.size()) x += 8;
        }
    }

    Qt::Edges hitEdges(QPointF point) const
    {
        Qt::Edges hit;
        if (selected.isEmpty() || !selected.adjusted(-6, -6, 6, 6).contains(point)) return hit;
        if (std::abs(point.x() - selected.left()) <= 6) hit |= Qt::LeftEdge;
        else if (std::abs(point.x() - selected.right()) <= 6) hit |= Qt::RightEdge;
        if (std::abs(point.y() - selected.top()) <= 6) hit |= Qt::TopEdge;
        else if (std::abs(point.y() - selected.bottom()) <= 6) hit |= Qt::BottomEdge;
        return hit;
    }

    void adjust(QPointF point)
    {
        selected = before;
        if (moving) {
            auto pos = before.topLeft() + point - start;
            selected.moveTo(std::clamp(pos.x(), 0., width() - before.width()),
                            std::clamp(pos.y(), 0., height() - before.height()));
        } else if (edges) {
            if (edges & Qt::LeftEdge) selected.setLeft(point.x());
            if (edges & Qt::RightEdge) selected.setRight(point.x());
            if (edges & Qt::TopEdge) selected.setTop(point.y());
            if (edges & Qt::BottomEdge) selected.setBottom(point.y());
        } else selected = QRectF(start, point);
        selected = selected.normalized().intersected(rect());
        update();
    }
};

class Panel : public QWidget {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.lumenshot.Panel")
public slots:
    Q_SCRIPTABLE void Dispatch(const QString &command)
    {
        if (command == "background") return;
        if (command == "present") { Present(); return; }
        if (capturing || QApplication::activeModalWidget() || !view->isEnabled()) return;
        if (command == "settings") {
            QTimer::singleShot(0, this, [this]{
                if (capturing || QApplication::activeModalWidget()) return;
                Present();
                showPreferences(this, hotkey, registered);
            });
        } else if (command == "region") grabDesktop(Mode::Region);
        else if (command == "screen") grabDesktop(Mode::Screen);
        else if (command == "window") grabDesktop(Mode::Window);
    }
    Q_SCRIPTABLE void Present()
    {
        if (auto *modal = QApplication::activeModalWidget()) {
            modal->raise();
            modal->activateWindow();
        } else if (!capturing) {
            showNormal();
            raise();
            activateWindow();
        }
    }
public:
    void closeEvent(QCloseEvent *event) override
    {
        event->ignore();
        hide();
    }
    void startDesktop()
    {
        const QString choice = shortcutChoice();
        registered = choice != "off" && hotkey.bind(choice);
        if (!registered && choice != "off" && qEnvironmentVariable("XDG_SESSION_TYPE") == "x11") {
            hint = "快捷键未生效";
            updateChrome();
        }
        connect(&hotkey, &Hotkey::triggered, this, [this]{ Dispatch("region"); });
        hotkeyRegion.bind("F1");
        hotkeyWindow.bind("F2");
        hotkeyScreen.bind("F3");
        connect(&hotkeyRegion, &Hotkey::triggered, this, [this]{ Dispatch("region"); });
        connect(&hotkeyWindow, &Hotkey::triggered, this, [this]{ Dispatch("window"); });
        connect(&hotkeyScreen, &Hotkey::triggered, this, [this]{ Dispatch("screen"); });
        auto *menu = new QMenu(this);
        menu->addAction("打开工作台", this, &Panel::Present)->setIcon(iconTint("focus", kIcon, 16));
        menu->addSeparator();
        menu->addAction("区域截图", this, [this]{ Dispatch("region"); })->setIcon(iconTint("scan", kIcon, 16));
        menu->addAction("整屏截图", this, [this]{ Dispatch("screen"); })->setIcon(iconTint("monitor", kIcon, 16));
        menu->addAction("窗口截图", this, [this]{ Dispatch("window"); })->setIcon(iconTint("focus", kIcon, 16));
        menu->addSeparator();
        menu->addAction("美化导出当前截图", this, [this]{ saveDecorated(); })
            ->setIcon(shapeIcon("sparkles", kIcon, 16));
        menu->addSeparator();
        menu->addAction("设置", this, [this]{ Dispatch("settings"); })->setIcon(iconTint("menu", kIcon, 16));
        menu->addAction("退出 Lumenshot", qApp, &QApplication::quit)->setIcon(iconTint("x", kIcon, 16));
        menuButton->setMenu(menu);
        tray.setIcon(brandIcon());
        tray.setToolTip("Lumenshot 截图工作台");
        tray.setContextMenu(menu);
        connect(&tray, &QSystemTrayIcon::activated, this, [this](auto reason){
            if (reason == QSystemTrayIcon::Trigger) Present();
        });
        tray.show();
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop");
        if (!settings.value("onboarded", false).toBool()) {
            settings.setValue("onboarded", true);
            QTimer::singleShot(1200, this, [this]{
                showToast("按 F1 随时开始截图", ToastKind::Info);
            });
        }
    }
    Panel()
    {
        setObjectName("panel");
        setWindowTitle("Lumenshot · 截图工作台");
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(true);
        setAcceptDrops(true);
        resize(740, 470);
        setMinimumSize(600, 400);
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop");
        delaySeconds = settings.value("captureDelay", 0).toInt();
        if (delaySeconds != 3 && delaySeconds != 5 && delaySeconds != 10) delaySeconds = 0;
        compactMode = settings.value("compactUI", false).toBool();
        if (compactMode) {
            pillH = 30;
            dockH = 42;
            toolBox = 30;
            toolIcon = 16;
            chromeBox = 26;
            edge = 10;
            resize(700, 440);
        }

        view = new Preview(this);
        view->scrim = true;
        view->selectable = true;
        view->setGeometry(rect());
        view->installEventFilter(this);

        fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);
        fadeAnimation->setDuration(170);
        fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);

        buildBrand();
        buildActions();
        buildDock();
        buildParams();
        buildToast();
        buildEmptyActions();
        addCardShadow(brandPill);
        addCardShadow(actionPill);
        addCardShadow(dock);

        connect(view, &Preview::edited, this, &Panel::updateChrome);
        connect(view, &Preview::cropped, this, [this]{ showToast("已裁剪", ToastKind::Success); });
        connect(view, &Preview::busy, this, [this](bool value){ busy = value; updateChrome(); });

        connect(region, &QPushButton::clicked, this, [this]{ grabDesktop(Mode::Region); });
        connect(windowMode, &QPushButton::clicked, this, [this]{ grabDesktop(Mode::Window); });
        connect(screenMode, &QPushButton::clicked, this, [this]{ grabDesktop(Mode::Screen); });

        auto *shapes = new QButtonGroup(this);
        const QList<QPair<QPushButton *, MaskShape>> toolsByShape = {
            {rectangle, MaskShape::Rectangle}, {ellipse, MaskShape::Ellipse}, {arrow, MaskShape::Arrow},
            {line, MaskShape::Line}, {highlighter, MaskShape::Highlight}, {number, MaskShape::Number},
            {label, MaskShape::Text}, {blur, MaskShape::Blur}, {mosaic, MaskShape::Mosaic},
            {cropTool, MaskShape::Crop}};
        for (const auto &[button, selected] : toolsByShape) {
            shapes->addButton(button);
            connect(button, &QPushButton::clicked, this, [this, selected]{ selectTool(selected); });
        }

        setSwatch(Qt::black);
        connect(swatch, &QPushButton::clicked, this, [this]{ openColorPopup(); });
        connect(strength, &QSlider::valueChanged, this, [this](int value){
            view->toolSize = value;
            paramValue->setText(QString::number(value));
        });
        connect(round, &QPushButton::toggled, this, [this](bool on){ view->circle = on; });
        connect(undo, &QPushButton::clicked, this, [this]{ undoEdit(); });
        connect(redo, &QPushButton::clicked, this, [this]{ redoEdit(); });

        connect(pin, &QPushButton::clicked, this, [this]{
            if (!pinned) pinned = new PinnedPreview;
            pinned->present(view->masks.image());
        });
        connect(copyButton, &QPushButton::clicked, this, [this]{ copyCurrent(); });
        connect(save, &QPushButton::clicked, this, [this]{ saveCurrent(); });
        connect(beautify, &QPushButton::clicked, this, [this]{ saveDecorated(); });
        connect(minButton, &QPushButton::clicked, this, &QWidget::showMinimized);
        connect(maxButton, &QPushButton::clicked, this, [this]{ toggleMaximized(); });
        connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
        connect(recentButton, &QPushButton::clicked, this, [this]{ openHistoryPopup(); });
        connect(pinnedListButton, &QPushButton::clicked, this, [this]{ openPinPopup(); });

        auto bind = [this](const QKeySequence &keys, std::function<void()> action) {
            auto *shortcut = new QShortcut(keys, this);
            shortcut->setContext(Qt::WindowShortcut);
            connect(shortcut, &QShortcut::activated, this, action);
        };
        bind(QKeySequence("1"), [this]{ selectTool(MaskShape::Rectangle); });
        bind(QKeySequence("2"), [this]{ selectTool(MaskShape::Ellipse); });
        bind(QKeySequence("3"), [this]{ selectTool(MaskShape::Arrow); });
        bind(QKeySequence("4"), [this]{ selectTool(MaskShape::Text); });
        bind(QKeySequence("5"), [this]{ selectTool(MaskShape::Blur); });
        bind(QKeySequence("6"), [this]{ selectTool(MaskShape::Mosaic); });
        bind(QKeySequence("7"), [this]{ selectTool(MaskShape::Line); });
        bind(QKeySequence("8"), [this]{ selectTool(MaskShape::Highlight); });
        bind(QKeySequence("9"), [this]{ selectTool(MaskShape::Number); });
        bind(QKeySequence("0"), [this]{ selectTool(MaskShape::Crop); });
        bind(QKeySequence::Undo, [this]{ undoEdit(); });
        bind(QKeySequence("Ctrl+Y"), [this]{ redoEdit(); });
        bind(QKeySequence("Ctrl+Shift+Z"), [this]{ redoEdit(); });
        bind(QKeySequence::Save, [this]{ saveCurrent(); });
        bind(QKeySequence::Copy, [this]{ copyCurrent(); });
        bind(QKeySequence::Paste, [this]{ pasteImage(); });
        bind(QKeySequence(Qt::Key_Return), [this]{ copyCurrent(); });
        bind(QKeySequence::Delete, [this]{ deleteSelected(); });
        bind(QKeySequence(Qt::Key_Escape), [this]{
            if (view->editing) {
                view->editing = false;
                view->selectedStep = -1;
                view->setCursor(Qt::ArrowCursor);
                updateChrome();
            } else {
                hide();
            }
        });

        setMinimumWidth(qMax(600, dock->sizeHint().width() + edge * 2));
        updateChrome();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const qreal radius = isMaximized() ? 0 : 14;
        const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(frame, radius, radius);
        p.fillPath(path, QColor("#09090B"));
        p.save();
        p.setClipPath(path);
        QLinearGradient sheen(0, 0, 0, 96);
        sheen.setColorAt(0, QColor(255, 255, 255, 12));
        sheen.setColorAt(1, QColor(255, 255, 255, 0));
        p.fillRect(QRectF(0, 0, width(), 96), sheen);
        QLinearGradient foot(0, height() - 76, 0, height());
        foot.setColorAt(0, QColor(0, 0, 0, 0));
        foot.setColorAt(1, QColor(0, 0, 0, 54));
        p.fillRect(QRectF(0, height() - 76, width(), 76), foot);
        p.restore();
        p.setPen(QPen(QColor(255, 255, 255, 20), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        if (!fadeAnimation) return;
        fadeAnimation->stop();
        setWindowOpacity(0.0);
        fadeAnimation->setStartValue(0.0);
        fadeAnimation->setEndValue(1.0);
        fadeAnimation->start();
    }
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (view) view->setGeometry(rect());
        layoutOverlays();
    }
    void changeEvent(QEvent *event) override
    {
        if (event->type() == QEvent::WindowStateChange && maxButton) {
            maxButton->setIcon(QIcon(controlGlyph(isMaximized() ? "restore" : "maximize",
                                                  QColor("#C9C9CF"))));
            update();
        }
        QWidget::changeEvent(event);
    }
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        const QMimeData *mime = event->mimeData();
        const bool file = mime->hasUrls() && !mime->urls().isEmpty()
            && QImageReader(mime->urls().first().toLocalFile()).canRead();
        if (mime->hasImage() || file) event->acceptProposedAction();
    }
    void dropEvent(QDropEvent *event) override
    {
        const QMimeData *mime = event->mimeData();
        QImage image;
        if (mime->hasImage()) image = qvariant_cast<QImage>(mime->imageData());
        else if (mime->hasUrls()) image.load(mime->urls().first().toLocalFile());
        if (image.isNull()) return;
        event->acceptProposedAction();
        loadImage(image);
        showToast("已载入图片", ToastKind::Success);
    }
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == view) {
            if (event->type() == QEvent::MouseMove) {
                auto *move = static_cast<QMouseEvent *>(event);
                if (!(move->buttons() & Qt::LeftButton)) {
                    const Qt::Edges edges = hitEdges(move->position().toPoint());
                    if (edges) view->setCursor(cursorForEdges(edges));
                    else view->setCursor(view->editing ? Qt::CrossCursor : Qt::ArrowCursor);
                }
            } else if (event->type() == QEvent::MouseButtonPress) {
                auto *press = static_cast<QMouseEvent *>(event);
                if (press->button() == Qt::LeftButton) {
                    const QPoint position = press->position().toPoint();
                    const Qt::Edges edges = hitEdges(position);
                    if (edges && !isMaximized() && windowHandle()) {
                        windowHandle()->startSystemResize(edges);
                        return true;
                    }
                    if (position.y() <= 56 && !view->editing && windowHandle()) {
                        windowHandle()->startSystemMove();
                        return true;
                    }
                }
            } else if (event->type() == QEvent::MouseButtonDblClick) {
                auto *press = static_cast<QMouseEvent *>(event);
                if (press->button() == Qt::LeftButton && press->position().y() <= 56 && !view->editing) {
                    toggleMaximized();
                    return true;
                }
            }
        } else if (watched == brandPill || watched == actionPill) {
            if (event->type() == QEvent::MouseButtonPress && windowHandle()) {
                windowHandle()->startSystemMove();
                return true;
            }
            if (event->type() == QEvent::MouseButtonDblClick) {
                toggleMaximized();
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

private:
    enum class Mode { Region, Screen, Window };
    bool capturing = false;
    bool busy = false;
    bool compactMode = false;
    int pillH = 34, dockH = 48, toolBox = 32, toolIcon = 17, chromeBox = 28, edge = 14;
    QString hint;
    Hotkey hotkey;
    Hotkey hotkeyRegion;
    Hotkey hotkeyWindow;
    Hotkey hotkeyScreen;
    bool registered = false;
    QSystemTrayIcon tray;
    QPointer<PinnedPreview> pinned;
    struct Shot { QByteArray data; QPixmap thumb; QString when; };
    QList<Shot> shots;
    Preview *view = nullptr;
    QPropertyAnimation *fadeAnimation = nullptr;
    QPropertyAnimation *paramsFade = nullptr;
    QPropertyAnimation *toastFade = nullptr;
    QPropertyAnimation *toastSlide = nullptr;
    QGraphicsOpacityEffect *paramsOpacity = nullptr;
    QGraphicsOpacityEffect *toastOpacity = nullptr;
    bool paramsVisible = false;
    QList<QColor> recentColors;

    QWidget *brandPill = nullptr;
    QWidget *actionPill = nullptr;
    QWidget *dock = nullptr;
    QWidget *params = nullptr;
    QWidget *toast = nullptr;
    QLabel *toastIcon = nullptr;
    QLabel *toastText = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *sizeLabel = nullptr;
    QLabel *paramLabel = nullptr;
    QLabel *paramValue = nullptr;
    QToolButton *menuButton = nullptr;
    QTimer toastTimer;

    QPushButton *pin = nullptr;
    QPushButton *copyButton = nullptr;
    QPushButton *save = nullptr;
    QPushButton *beautify = nullptr;
    QPushButton *minButton = nullptr;
    QPushButton *maxButton = nullptr;
    QPushButton *closeButton = nullptr;

    QPushButton *region = nullptr;
    QPushButton *windowMode = nullptr;
    QPushButton *screenMode = nullptr;
    QToolButton *delayButton = nullptr;
    int delaySeconds = 0;
    QPushButton *rectangle = nullptr;
    QPushButton *ellipse = nullptr;
    QPushButton *arrow = nullptr;
    QPushButton *line = nullptr;
    QPushButton *highlighter = nullptr;
    QPushButton *number = nullptr;
    QPushButton *label = nullptr;
    QPushButton *blur = nullptr;
    QPushButton *mosaic = nullptr;
    QPushButton *cropTool = nullptr;
    ToolCapsule *toolCapsule = nullptr;
    QWidget *emptyActions = nullptr;
    QPushButton *swatch = nullptr;
    QPushButton *round = nullptr;
    QPushButton *undo = nullptr;
    QPushButton *redo = nullptr;
    QPushButton *recentButton = nullptr;
    QPushButton *pinnedListButton = nullptr;
    QSlider *strength = nullptr;

    QPushButton *chromeButton(const QIcon &symbol, const QString &tip)
    {
        auto *button = new HoverButton(this);
        button->setIcon(symbol);
        button->setIconSize({16, 16});
        button->setFixedSize(chromeBox, chromeBox);
        button->setToolTip(tip);
        button->setAccessibleName(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    }

    QPushButton *modeButton(const QString &symbol, const QString &text, const QString &tip, bool primary = false)
    {
        auto *button = new HoverButton(this);
        button->setStateIcons(iconTint(symbol, primary ? kMint : QColor("#A6A6AE"), 15),
                              iconTint(symbol, primary ? QColor("#C9FBE6") : QColor("#FFFFFF"), 15));
        button->setText(text);
        button->setObjectName(primary ? "modePrimary" : "mode");
        button->setIconSize({15, 15});
        button->setFixedHeight(pillH);
        button->setToolTip(tip);
        button->setAccessibleName(text + "截图");
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    }

    QPushButton *toolButton(const QString &symbol, const QString &tip)
    {
        auto *button = new HoverButton(this);
        button->setCheckable(true);
        button->setFixedSize(toolBox, toolBox);
        button->setIconSize({toolIcon, toolIcon});
        button->setToolTip(tip);
        button->setAccessibleName(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        button->setProperty("iconName", symbol);
        button->setStateIcons(iconTint(symbol, kIcon, toolIcon),
                              iconTint(symbol, QColor("#FFFFFF"), toolIcon));
        button->setEnabled(false);
        return button;
    }

    QPushButton *shapeToolButton(const QString &kind, const QString &tip)
    {
        auto *button = new HoverButton(this);
        button->setCheckable(true);
        button->setFixedSize(toolBox, toolBox);
        button->setIconSize({toolIcon, toolIcon});
        button->setToolTip(tip);
        button->setAccessibleName(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        button->setProperty("iconName", kind);
        button->setProperty("glyph", true);
        button->setStateIcons(shapeIcon(kind, kIcon, toolIcon),
                              shapeIcon(kind, QColor("#FFFFFF"), toolIcon));
        button->setEnabled(false);
        return button;
    }

    QPushButton *historyButton(const QString &symbol, const QString &tip)
    {
        auto *button = new HoverButton(this);
        button->setFixedSize(toolBox, toolBox);
        button->setIconSize({toolIcon, toolIcon});
        button->setToolTip(tip);
        button->setAccessibleName(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        button->setProperty("iconName", symbol);
        button->setStateIcons(iconTint(symbol, kIcon, toolIcon),
                              iconTint(symbol, QColor("#FFFFFF"), toolIcon));
        button->setEnabled(false);
        return button;
    }

    void buildBrand()
    {
        brandPill = new QWidget(this);
        brandPill->setObjectName("pill");
        brandPill->setAttribute(Qt::WA_StyledBackground, true);
        brandPill->setFixedHeight(pillH);
        brandPill->installEventFilter(this);
        auto *row = new QHBoxLayout(brandPill);
        row->setContentsMargins(7, 0, 7, 0);
        row->setSpacing(8);

        auto *logo = new QLabel;
        logo->setPixmap(brandOrb(22));
        logo->setFixedSize(22, 22);
        logo->setAttribute(Qt::WA_TransparentForMouseEvents);
        row->addWidget(logo);

        auto *name = new QLabel("Lumenshot");
        name->setObjectName("brand");
        QFont brandFont = name->font();
        brandFont.setPixelSize(12);
        brandFont.setWeight(QFont::Medium);
        name->setFont(brandFont);
        name->setAttribute(Qt::WA_TransparentForMouseEvents);
        row->addWidget(name);
        row->addWidget(separator(14));

        auto *dot = new QLabel;
        dot->setFixedSize(5, 5);
        dot->setStyleSheet("background:#A3F0CE;border-radius:2px;");
        dot->setAttribute(Qt::WA_TransparentForMouseEvents);
        row->addWidget(dot);

        statusLabel = new QLabel("等待截图");
        statusLabel->setObjectName("status");
        statusLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        row->addWidget(statusLabel);

        sizeLabel = new QLabel;
        sizeLabel->setObjectName("size");
        sizeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        QFont numeric = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        numeric.setPixelSize(10);
        sizeLabel->setFont(numeric);
        sizeLabel->hide();
        row->addWidget(sizeLabel);

        recentButton = new HoverButton(this);
        recentButton->setFixedSize(26, 26);
        recentButton->setIconSize({15, 15});
        static_cast<HoverButton *>(recentButton)->setStateIcons(shapeIcon("history", QColor("#9A9AA2"), 15),
                                                                shapeIcon("history", kMint, 15));
        recentButton->setToolTip("最近截图");
        recentButton->setAccessibleName("最近截图");
        recentButton->setCursor(Qt::PointingHandCursor);
        recentButton->setFocusPolicy(Qt::NoFocus);
        row->addWidget(recentButton);

        pinnedListButton = new HoverButton(this);
        pinnedListButton->setFixedSize(26, 26);
        pinnedListButton->setIconSize({15, 15});
        static_cast<HoverButton *>(pinnedListButton)->setStateIcons(
            iconTint("pin", QColor("#9A9AA2"), 15), iconTint("pin", kMint, 15));
        pinnedListButton->setToolTip("贴图管理");
        pinnedListButton->setAccessibleName("贴图管理");
        pinnedListButton->setCursor(Qt::PointingHandCursor);
        pinnedListButton->setFocusPolicy(Qt::NoFocus);
        row->addWidget(pinnedListButton);

        menuButton = new QToolButton;
        menuButton->setIcon(iconTint("menu", QColor("#9A9AA2"), 16));
        menuButton->setIconSize({16, 16});
        menuButton->setFixedSize(26, 26);
        menuButton->setToolTip("菜单");
        menuButton->setAccessibleName("菜单");
        menuButton->setPopupMode(QToolButton::InstantPopup);
        menuButton->setCursor(Qt::PointingHandCursor);
        menuButton->setFocusPolicy(Qt::NoFocus);
        row->addWidget(menuButton);
    }

    void buildActions()
    {
        actionPill = new QWidget(this);
        actionPill->setObjectName("pill");
        actionPill->setAttribute(Qt::WA_StyledBackground, true);
        actionPill->setFixedHeight(pillH);
        actionPill->installEventFilter(this);
        auto *row = new QHBoxLayout(actionPill);
        row->setContentsMargins(5, 0, 5, 0);
        row->setSpacing(2);

        pin = chromeButton(iconTint("pin", kIcon, 16), "置顶显示当前截图");
        copyButton = chromeButton(iconTint("copy", kIcon, 16), "复制到剪贴板 · Ctrl+C");
        save = chromeButton(iconTint("download", kIcon, 16), "保存图片 · Ctrl+S");
        beautify = chromeButton(shapeIcon("sparkles", kIcon, 16), "美化导出 · 圆角背景");
        row->addWidget(pin);
        row->addWidget(copyButton);
        row->addWidget(save);
        row->addWidget(beautify);
        row->addWidget(separator(14));

        minButton = chromeButton(QIcon(controlGlyph("minimize", QColor("#C9C9CF"))), "最小化");
        maxButton = chromeButton(QIcon(controlGlyph("maximize", QColor("#C9C9CF"))), "最大化");
        closeButton = chromeButton(QIcon(controlGlyph("close", QColor("#C9C9CF"))), "关闭窗口（收起到托盘）");
        static_cast<HoverButton *>(closeButton)->setHoverTint(QColor(255, 69, 58), 62);
        row->addWidget(minButton);
        row->addWidget(maxButton);
        row->addWidget(closeButton);
    }

    void buildDock()
    {
        dock = new QWidget(this);
        dock->setObjectName("dock");
        dock->setAttribute(Qt::WA_StyledBackground, true);
        dock->setFixedHeight(dockH);
        auto *row = new QHBoxLayout(dock);
        row->setContentsMargins(8, 0, 8, 0);
        row->setSpacing(3);

        region = modeButton("scan", "区域", "拖动鼠标选择当前屏幕的一部分", true);
        windowMode = modeButton("focus", "窗口", "点击当前屏幕中完整可见的窗口");
        screenMode = modeButton("monitor", "整屏", "截取工作台所在的显示器");
        row->addWidget(region);
        row->addWidget(windowMode);
        row->addWidget(screenMode);

        delayButton = new QToolButton(dock);
        delayButton->setIcon(shapeIcon("clock", QColor("#A6A6AE"), toolIcon - 2));
        delayButton->setIconSize({toolIcon - 2, toolIcon - 2});
        delayButton->setFixedSize(toolBox - 2, toolBox);
        delayButton->setCursor(Qt::PointingHandCursor);
        delayButton->setFocusPolicy(Qt::NoFocus);
        delayButton->setPopupMode(QToolButton::InstantPopup);
        auto *delayMenu = new QMenu(delayButton);
        auto *delayGroup = new QActionGroup(delayMenu);
        delayGroup->setExclusive(true);
        for (int seconds : {0, 3, 5, 10}) {
            auto *action = delayMenu->addAction(seconds == 0 ? "立即截图"
                                                             : QString("%1 秒后截图").arg(seconds));
            action->setCheckable(true);
            action->setChecked(seconds == delaySeconds);
            delayGroup->addAction(action);
            connect(action, &QAction::triggered, this, [this, seconds]{
                delaySeconds = seconds;
                QSettings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop")
                    .setValue("captureDelay", seconds);
                delayButton->setToolTip(seconds == 0 ? "延时截图 · 立即"
                                                     : QString("延时截图 · %1 秒").arg(seconds));
            });
        }
        delayButton->setMenu(delayMenu);
        delayButton->setToolTip(delaySeconds == 0 ? "延时截图 · 立即"
                                                  : QString("延时截图 · %1 秒").arg(delaySeconds));
        row->addWidget(delayButton);
        row->addWidget(separator(18));

        rectangle = toolButton("rectangle-horizontal", "矩形遮挡 · 1");
        ellipse = toolButton("circle", "椭圆遮挡 · 2");
        arrow = toolButton("move-up-right", "箭头 · 3");
        line = shapeToolButton("line", "直线 · 7");
        highlighter = shapeToolButton("highlighter", "高亮笔 · 8");
        number = shapeToolButton("number", "序号标记 · 9");
        label = toolButton("type", "文字 · 4");
        blur = toolButton("blend", "模糊 · 5");
        mosaic = toolButton("grid-3x3", "马赛克 · 6");
        cropTool = shapeToolButton("crop", "裁剪 · 0");
        for (auto *button : {rectangle, ellipse, arrow, line, highlighter, number,
                             label, blur, mosaic, cropTool}) row->addWidget(button);
        row->addWidget(separator(18));

        swatch = new HoverButton(this);
        swatch->setFixedSize(toolBox - 2, toolBox - 2);
        swatch->setIconSize({toolIcon + 3, toolIcon + 3});
        swatch->setToolTip("遮挡颜色");
        swatch->setAccessibleName("遮挡颜色");
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setFocusPolicy(Qt::NoFocus);
        swatch->setIcon(swatchIcon(Qt::black, toolIcon + 3));
        row->addWidget(swatch);
        row->addWidget(separator(18));

        undo = historyButton("undo-2", "撤销 · Ctrl+Z");
        redo = historyButton("redo-2", "重做 · Ctrl+Y");
        row->addWidget(undo);
        row->addWidget(redo);
        row->addWidget(separator(18));

        toolCapsule = new ToolCapsule(dock);
        toolCapsule->lower();
        toolCapsule->hide();
    }

    QPushButton *toolFor(MaskShape shape) const
    {
        switch (shape) {
        case MaskShape::Rectangle: return rectangle;
        case MaskShape::Ellipse: return ellipse;
        case MaskShape::Arrow: return arrow;
        case MaskShape::Line: return line;
        case MaskShape::Highlight: return highlighter;
        case MaskShape::Number: return number;
        case MaskShape::Text: return label;
        case MaskShape::Blur: return blur;
        case MaskShape::Mosaic: return mosaic;
        case MaskShape::Crop: return cropTool;
        }
        return nullptr;
    }

    void placeToolCapsule(bool animate)
    {
        if (!toolCapsule) return;
        QPushButton *active = view->editing ? toolFor(view->shape) : nullptr;
        if (!active || !active->isEnabled()) {
            toolCapsule->hide();
            return;
        }
        const QRect box(active->mapTo(dock, QPoint(0, 0)), active->size());
        toolCapsule->show();
        toolCapsule->glideTo(box.adjusted(1, 1, -1, -1), animate);
    }

    void selectTool(MaskShape shape)
    {
        if (!view || view->masks.image().isNull() || busy) return;
        view->shape = shape;
        view->editing = true;
        view->selectedStep = -1;
        view->setCursor(Qt::CrossCursor);
        if (auto *button = toolFor(shape)) button->setChecked(true);
        switch (shape) {
        case MaskShape::Arrow: paramLabel->setText("线宽"); strength->setRange(1, 64); strength->setValue(4); break;
        case MaskShape::Line: paramLabel->setText("线宽"); strength->setRange(1, 32); strength->setValue(4); break;
        case MaskShape::Highlight: paramLabel->setText("粗细"); strength->setRange(6, 48); strength->setValue(18); break;
        case MaskShape::Number: paramLabel->setText("字号"); strength->setRange(16, 64); strength->setValue(28); break;
        case MaskShape::Text: paramLabel->setText("字号"); strength->setRange(1, 128); strength->setValue(28); break;
        case MaskShape::Blur:
        case MaskShape::Mosaic: paramLabel->setText("力度"); strength->setRange(1, 64); strength->setValue(12); break;
        default: break;
        }
        updateChrome();
        placeToolCapsule(true);
    }

    void buildParams()
    {
        params = new QWidget(this);
        params->setObjectName("params");
        params->setAttribute(Qt::WA_StyledBackground, true);
        params->setFixedHeight(pillH);
        auto *row = new QHBoxLayout(params);
        row->setContentsMargins(12, 0, 10, 0);
        row->setSpacing(8);

        paramLabel = new QLabel("力度");
        paramLabel->setObjectName("paramLabel");
        strength = new QSlider(Qt::Horizontal);
        strength->setRange(1, 64);
        strength->setValue(12);
        strength->setFixedWidth(84);
        strength->setFocusPolicy(Qt::NoFocus);
        paramValue = new QLabel("12");
        paramValue->setObjectName("paramValue");
        paramValue->setFixedWidth(20);
        paramValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont numeric = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        numeric.setPixelSize(10);
        paramValue->setFont(numeric);
        round = new HoverButton(this);
        round->setText("正圆");
        round->setObjectName("round");
        round->setCheckable(true);
        round->setFixedHeight(24);
        round->setCursor(Qt::PointingHandCursor);
        round->setFocusPolicy(Qt::NoFocus);
        round->hide();

        row->addWidget(paramLabel);
        row->addWidget(strength);
        row->addWidget(paramValue);
        row->addWidget(round);
        params->hide();

        paramsOpacity = new QGraphicsOpacityEffect(params);
        paramsOpacity->setOpacity(0.0);
        params->setGraphicsEffect(paramsOpacity);
        paramsFade = new QPropertyAnimation(paramsOpacity, "opacity", this);
        paramsFade->setDuration(140);
        paramsFade->setEasingCurve(QEasingCurve::OutCubic);
        connect(paramsFade, &QPropertyAnimation::finished, this, [this]{
            if (!paramsVisible) params->hide();
        });
    }

    void buildEmptyActions()
    {
        emptyActions = new QWidget(this);
        auto *column = new QVBoxLayout(emptyActions);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(9);

        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        struct Entry { QString symbol; QString text; Mode mode; };
        const QList<Entry> entries = {
            {"scan", "区域截图", Mode::Region},
            {"focus", "窗口截图", Mode::Window},
            {"monitor", "整屏截图", Mode::Screen}};
        for (const auto &entry : entries) {
            auto *button = new HoverButton(emptyActions);
            button->setObjectName("emptyAction");
            button->setStateIcons(iconTint(entry.symbol, kIcon, 14),
                                  iconTint(entry.symbol, QColor("#FFFFFF"), 14));
            button->setText(entry.text);
            button->setIconSize({14, 14});
            button->setFixedHeight(34);
            button->setCursor(Qt::PointingHandCursor);
            button->setFocusPolicy(Qt::NoFocus);
            button->setToolTip(entry.text);
            connect(button, &QPushButton::clicked, this, [this, mode = entry.mode]{ grabDesktop(mode); });
            row->addWidget(button);
        }
        column->addLayout(row);

        auto *tip = new QLabel("1–6 选工具 · Enter 复制 · Ctrl+S 保存");
        tip->setObjectName("emptyHint");
        tip->setAlignment(Qt::AlignCenter);
        column->addWidget(tip);
        emptyActions->hide();
    }

    void buildToast()
    {
        toast = new QWidget(this);
        toast->setObjectName("toast");
        toast->setAttribute(Qt::WA_StyledBackground, true);
        toast->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        toast->setFixedHeight(30);
        auto *row = new QHBoxLayout(toast);
        row->setContentsMargins(11, 0, 12, 0);
        row->setSpacing(6);
        toastIcon = new QLabel(toast);
        toastIcon->setFixedWidth(12);
        toastIcon->setAlignment(Qt::AlignCenter);
        toastText = new QLabel(toast);
        row->addWidget(toastIcon);
        row->addWidget(toastText);
        toast->hide();
        toastOpacity = new QGraphicsOpacityEffect(toast);
        toastOpacity->setOpacity(0.0);
        toast->setGraphicsEffect(toastOpacity);
        toastFade = new QPropertyAnimation(toastOpacity, "opacity", this);
        toastFade->setDuration(150);
        toastFade->setEasingCurve(QEasingCurve::OutCubic);
        toastSlide = new QPropertyAnimation(toast, "pos", this);
        toastSlide->setDuration(170);
        toastSlide->setEasingCurve(QEasingCurve::OutCubic);
        connect(toastFade, &QPropertyAnimation::finished, this, [this]{
            if (toastOpacity->opacity() < 0.01) toast->hide();
        });
        toastTimer.setSingleShot(true);
        toastTimer.setInterval(1800);
        connect(&toastTimer, &QTimer::timeout, this, [this]{ hideToast(); });
    }

    void toggleMaximized()
    {
        if (isMaximized()) showNormal();
        else showMaximized();
    }

    void rememberShot(const QImage &image)
    {
        if (image.isNull()) return;
        Shot shot;
        shot.when = QDateTime::currentDateTime().toString("MM-dd HH:mm:ss");
        shot.thumb = QPixmap::fromImage(image.scaled(QSize(128, 86), Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation));
        QBuffer buffer(&shot.data);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        shots.prepend(shot);
        while (shots.size() > 5) shots.removeLast();
    }

    void loadImage(const QImage &image, bool remember = true)
    {
        if (image.isNull()) return;
        view->masks.reset(image);
        view->selectedStep = -1;
        view->editing = false;
        view->setCursor(Qt::ArrowCursor);
        if (remember) rememberShot(image);
        updateChrome();
    }

    void pasteImage()
    {
        const QImage image = QApplication::clipboard()->image();
        if (image.isNull()) {
            showToast("剪贴板里没有图片", ToastKind::Warning);
            return;
        }
        loadImage(image);
        showToast("已粘贴图片", ToastKind::Success);
    }

    void openPinPopup()
    {
        int count = 0;
        for (const QPointer<PinnedPreview> &pin : livePins()) if (pin) ++count;
        pinnedListButton->setToolTip(count > 0
            ? QStringLiteral("贴图管理（%1 张）").arg(count) : QStringLiteral("贴图管理"));
        auto *popup = new PinPopup(this);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->adjustSize();
        const QPoint anchor = pinnedListButton->mapToGlobal(QPoint(0, 0));
        QPoint target(anchor.x() + (pinnedListButton->width() - popup->width()) / 2,
                      anchor.y() - popup->height() - 10);
        const QRect available = screen()->availableGeometry();
        target.setX(qBound(available.left() + 6, target.x(), available.right() - popup->width() - 6));
        target.setY(qBound(available.top() + 6, target.y(), available.bottom() - popup->height() - 6));
        popup->move(target);
        popup->show();
    }

    void openHistoryPopup()
    {
        if (shots.isEmpty()) {
            showToast("还没有截图记录", ToastKind::Warning);
            return;
        }
        QList<QPair<QPixmap, QString>> items;
        for (const auto &shot : shots) items.append({shot.thumb, shot.when});
        auto *popup = new ShotPopup(items, [this](int index){
            if (index < 0 || index >= shots.size()) return;
            const QImage image = QImage::fromData(shots.at(index).data, "PNG");
            if (image.isNull()) return;
            loadImage(image, false);
            showToast("已恢复最近截图", ToastKind::Success);
        }, this);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->adjustSize();
        const QPoint anchor = recentButton->mapToGlobal(QPoint(0, 0));
        QPoint target(anchor.x() + (recentButton->width() - popup->width()) / 2,
                      anchor.y() - popup->height() - 10);
        const QRect available = screen()->availableGeometry();
        target.setX(qBound(available.left() + 6, target.x(), available.right() - popup->width() - 6));
        target.setY(qBound(available.top() + 6, target.y(), available.bottom() - popup->height() - 6));
        popup->move(target);
        popup->show();
    }

    void copyCurrent()
    {
        if (!view || view->masks.image().isNull() || busy) return;
        QApplication::clipboard()->setImage(view->masks.image());
        showToast("已复制到剪贴板", ToastKind::Success);
    }

    void saveCurrent()
    {
        if (!view || view->masks.image().isNull() || busy) return;
        if (saveImage(this, view->masks.image())) showToast("已保存图片", ToastKind::Success);
    }

    void saveDecorated()
    {
        if (!view || view->masks.image().isNull() || busy) return;
        if (saveImage(this, decoratedImage(view->masks.image())))
            showToast("已保存美化图片", ToastKind::Success);
    }

    void undoEdit()
    {
        if (!view || !view->masks.canUndo() || busy) return;
        view->edit([](Masks &model){ model.undo(); }, view->masks.isLarge());
    }

    void redoEdit()
    {
        if (!view || !view->masks.canRedo() || busy) return;
        view->edit([](Masks &model){ model.redo(); }, view->masks.isLarge());
    }

    void deleteSelected()
    {
        if (!view || busy) return;
        const int index = view->selectedStep;
        if (index < 0 || index >= view->masks.stepCount()) return;
        view->selectedStep = -1;
        view->edit([index](Masks &model){ model.removeStep(index); }, view->masks.isLarge());
        showToast("已删除标注", ToastKind::Success);
    }

    void setSwatch(const QColor &value)
    {
        view->color = value;
        swatch->setIcon(swatchIcon(value, toolIcon + 3));
        swatch->setToolTip(QString("遮挡颜色 %1").arg(value.name()));
        if (view->selectedStep >= 0 && view->selectedStep < view->masks.stepCount()) {
            const int index = view->selectedStep;
            view->edit([index, value](Masks &model){ model.setStepColor(index, value); },
                       view->masks.isLarge());
        }
    }

    void rememberColor(const QColor &value)
    {
        recentColors.removeAll(value);
        recentColors.prepend(value);
        while (recentColors.size() > 5) recentColors.removeLast();
    }

    ColorPopup *makeColorPopup()
    {
        return new ColorPopup(view->color, recentColors,
            [this](const QColor &value){ setSwatch(value); },
            [this]{
                QColorDialog dialog(view->color, this);
                dialog.setWindowTitle("选择遮挡颜色");
                dialog.setOption(QColorDialog::DontUseNativeDialog);
                if (dialog.exec() == QDialog::Accepted) {
                    const QColor value = dialog.currentColor();
                    if (value.isValid()) {
                        setSwatch(value);
                        rememberColor(value);
                    }
                }
            }, this);
    }

    void openColorPopup()
    {
        auto *popup = makeColorPopup();
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->adjustSize();
        const QPoint anchor = swatch->mapToGlobal(QPoint(0, 0));
        QPoint target(anchor.x() + (swatch->width() - popup->width()) / 2,
                      anchor.y() - popup->height() - 10);
        const QRect available = screen()->availableGeometry();
        target.setX(qBound(available.left() + 6, target.x(), available.right() - popup->width() - 6));
        target.setY(qBound(available.top() + 6, target.y(), available.bottom() - popup->height() - 6));
        popup->move(target);
        popup->show();
    }

    void setParamsVisible(bool on)
    {
        if (!params || on == paramsVisible) return;
        paramsVisible = on;
        paramsFade->stop();
        if (on) {
            params->show();
            layoutOverlays();
            paramsOpacity->setOpacity(0.0);
            paramsFade->setStartValue(0.0);
            paramsFade->setEndValue(1.0);
        } else {
            paramsFade->setStartValue(paramsOpacity->opacity());
            paramsFade->setEndValue(0.0);
        }
        paramsFade->start();
    }

    void hideToast()
    {
        if (!toast->isVisible()) return;
        toastFade->stop();
        toastSlide->stop();
        toastFade->setStartValue(toastOpacity->opacity());
        toastFade->setEndValue(0.0);
        toastSlide->setStartValue(toast->pos());
        toastSlide->setEndValue(toast->pos() + QPoint(0, -6));
        toastFade->start();
        toastSlide->start();
    }

    Qt::Edges hitEdges(const QPoint &position) const
    {
        if (isMaximized()) return {};
        return edgeHit(size(), position);
    }

    void applyIcons()
    {
        for (auto *button : {rectangle, ellipse, arrow, line, highlighter, number,
                             label, blur, mosaic, cropTool, undo, redo}) {
            const QString name = button->property("iconName").toString();
            if (name.isEmpty()) continue;
            const bool checked = button->isCheckable() && button->isChecked();
            const bool glyph = button->property("glyph").toBool();
            auto art = [&](const QColor &color) {
                return glyph ? shapeIcon(name, color, toolIcon) : iconTint(name, color, toolIcon);
            };
            static_cast<HoverButton *>(button)->setStateIcons(
                art(checked ? kMint : kIcon), art(checked ? kMint : QColor("#FFFFFF")));
        }
    }

    void updateChrome()
    {
        if (!view || !statusLabel) return;
        const bool hasImage = !view->masks.image().isNull();
        const bool active = hasImage && !busy;
        if (view->selectedStep >= view->masks.stepCount()) view->selectedStep = -1;
        const bool selected = view->selectedStep >= 0;

        statusLabel->setText(!hasImage && !hint.isEmpty() ? hint
                             : (selected ? "已选中标注"
                                         : (hasImage ? (view->editing ? "标注中 · Esc 退出" : "已截取")
                                                     : "等待截图")));
        statusLabel->setToolTip(hint.isEmpty() ? QString() : "快捷键不可用，可在设置中换键");
        if (hasImage) {
            sizeLabel->setText(QString("%1 × %2")
                                   .arg(view->masks.image().width())
                                   .arg(view->masks.image().height()));
        }
        sizeLabel->setVisible(hasImage);
        emptyActions->setVisible(!hasImage);

        for (auto *button : {pin, copyButton, save}) {
            button->setVisible(hasImage);
            button->setEnabled(active);
        }
        for (auto *button : {region, windowMode, screenMode}) button->setEnabled(!busy);
        for (auto *button : {rectangle, ellipse, arrow, line, highlighter, number,
                             label, blur, mosaic, cropTool}) button->setEnabled(active);
        const bool colorTool = view->shape == MaskShape::Rectangle || view->shape == MaskShape::Ellipse
            || view->shape == MaskShape::Arrow || view->shape == MaskShape::Line
            || view->shape == MaskShape::Highlight || view->shape == MaskShape::Number
            || view->shape == MaskShape::Text;
        swatch->setEnabled(active && colorTool);
        undo->setEnabled(active && view->masks.canUndo());
        redo->setEnabled(active && view->masks.canRedo());
        applyIcons();

        QPushButton *activeTool = view->editing ? toolFor(view->shape) : nullptr;
        if (!active || !activeTool || !activeTool->isEnabled()) toolCapsule->hide();
        else if (!toolCapsule->isVisible()) placeToolCapsule(false);

        const bool needsParams = active && view->editing &&
            (view->shape == MaskShape::Ellipse || view->shape == MaskShape::Arrow ||
             view->shape == MaskShape::Text || view->shape == MaskShape::Blur ||
             view->shape == MaskShape::Mosaic);
        if (needsParams) {
            const bool isEllipse = view->shape == MaskShape::Ellipse;
            paramLabel->setVisible(!isEllipse);
            strength->setVisible(!isEllipse);
            paramValue->setVisible(!isEllipse);
            round->setVisible(isEllipse);
        }
        setParamsVisible(needsParams);
        layoutOverlays();
    }

    enum class ToastKind { Info, Success, Warning };

    void showToast(const QString &text, ToastKind kind = ToastKind::Info)
    {
        toastFade->stop();
        toastSlide->stop();
        toastText->setText(text);
        QString color = "#8E8E97", glyph = "•";
        int glyphSize = 14;
        if (kind == ToastKind::Success) {
            color = "#A3F0CE";
            glyph = "✓";
            glyphSize = 12;
        } else if (kind == ToastKind::Warning) {
            color = "#F0C97E";
            glyph = "!";
            glyphSize = 13;
        }
        toastIcon->setText(glyph);
        toastIcon->setStyleSheet(QString("color:%1;font-size:%2px;font-weight:600;background:transparent;")
                                 .arg(color).arg(glyphSize));
        toast->adjustSize();
        const QPoint target((width() - toast->width()) / 2, 58);
        if (!toast->isVisible()) {
            toast->move(target.x(), target.y() - 6);
            toastOpacity->setOpacity(0.0);
            toastFade->setStartValue(0.0);
        } else {
            toastFade->setStartValue(toastOpacity->opacity());
        }
        toast->show();
        toast->raise();
        toastFade->setEndValue(1.0);
        toastSlide->setStartValue(toast->pos());
        toastSlide->setEndValue(target);
        toastSlide->start();
        toastFade->start();
        toastTimer.start();
    }

    void layoutOverlays()
    {
        if (!brandPill) return;
        brandPill->adjustSize();
        brandPill->move(edge, edge);
        actionPill->adjustSize();
        actionPill->move(width() - edge - actionPill->width(), edge);
        dock->adjustSize();
        dock->move((width() - dock->width()) / 2, height() - (edge + 2) - dock->height());
        if (params->isVisible()) {
            params->adjustSize();
            params->move((width() - params->width()) / 2, dock->y() - 8 - params->height());
        }
        if (toast->isVisible()) {
            toast->adjustSize();
            toast->move((width() - toast->width()) / 2, 58);
        }
        if (!emptyActions->isHidden()) {
            emptyActions->adjustSize();
            emptyActions->move((width() - emptyActions->width()) / 2, view->height() / 2 + 48);
        }
        for (auto *widget : {static_cast<QWidget *>(brandPill), static_cast<QWidget *>(actionPill),
                             static_cast<QWidget *>(dock), static_cast<QWidget *>(params),
                             static_cast<QWidget *>(toast), static_cast<QWidget *>(emptyActions)}) {
            widget->raise();
        }
    }

    void grabDesktop(Mode mode)
    {
        if (capturing) return;
        if (QGuiApplication::platformName() != "xcb" || qEnvironmentVariable("XDG_SESSION_TYPE") != "x11") {
            QMessageBox::information(this, "桌面截图", "当前版本的截图面板支持 X11 桌面。");
            return;
        }
        QPointer<QScreen> display = screen();
        const bool wasVisible = isVisible();
        capturing = true;
        const bool restorePin = pinned && pinned->isVisible();
        if (restorePin) pinned->hide();
        hide();
        // Allow GNOME's hide animation to finish before sampling the desktop.
        QTimer::singleShot(wasVisible || restorePin ? 320 : 0, this, [this, display, mode, restorePin, wasVisible]{
            if (delaySeconds > 0) {
                Countdown countdown(display ? display->geometry() : QRect(), delaySeconds);
                if (countdown.exec() != QDialog::Accepted) {
                    capturing = false;
                    if (restorePin && pinned) pinned->show();
                    if (wasVisible) show();
                    return;
                }
            }
            QDBusInterface lock("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver",
                                "org.gnome.ScreenSaver", QDBusConnection::sessionBus());
            QDBusReply<bool> active = lock.call("GetActive");
            QImage image;
            QList<WindowFrame> frames;
            if (!active.isValid() || active.value()) {
                capturing = false;
                if (restorePin && pinned) pinned->show();
                if (wasVisible) show();
                showToast("桌面已锁定或无法确认解锁状态", ToastKind::Warning);
                return;
            }
            if (display) {
                if (mode == Mode::Window)
                    frames = captureWindowFrames(display, image);
                else
                    image = display->grabWindow(0).toImage();
            }
            if (image.isNull()) {
                capturing = false;
                if (restorePin && pinned) pinned->show();
                show();
                QMessageBox::warning(this, "截图失败", "请确认桌面已解锁且显示器仍连接，再重新截图。");
                return;
            }
            bool failed = false;
            QString failure;
            if (display) {
                const QRect full = mode == Mode::Screen ? image.rect() : QRect();
                Selection select(image, display, mode == Mode::Window, frames, full);
                select.onCaptured = [this](const QImage &shot){ rememberShot(shot); };
                select.exec();
                failure = select.failure;
                failed = !failure.isEmpty();
            }
            capturing = false;
            if (restorePin && pinned) pinned->show();
            if (failed) {
                show();
                raise();
                activateWindow();
                showToast(failure, ToastKind::Warning);
                return;
            }
            if (wasVisible) {
                show();
                raise();
                activateWindow();
            }
        });
    }
};
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("Lumenshot");
    app.setDesktopFileName("lumenshot");
    app.setWindowIcon(brandIcon());
    app.setStyle("Fusion");
    app.setPalette(darkPalette());
    app.setStyleSheet(R"(
        QWidget {
            color:#F4F4F5;
            font-family:"Noto Sans CJK SC","Noto Sans SC",sans-serif;
            font-size:13px;
        }
        QToolTip {
            background:#1B1B1F; color:#E9E9EC;
            border:1px solid rgba(255,255,255,26);
            border-radius:6px; padding:5px 8px;
        }
        QPushButton {
            background:#1B1B1F; color:#E9E9EC;
            border:1px solid #2E2E35; border-radius:8px;
            padding:6px 12px;
        }
        QPushButton:hover { background:#24242A; }
        QPushButton:pressed { background:#17171A; }
        QPushButton:disabled { background:#151517; color:#6B6B73; border-color:#232328; }
        QMenu { background:#1A1A1E; border:1px solid #2C2C32; border-radius:8px; padding:4px; }
        QMenu::item { padding:6px 22px; border-radius:6px; }
        QMenu::item:selected { background:#2A2A31; }
        QMenu::item:disabled { color:#6B6B73; }
        QMenu::separator { height:1px; background:#2C2C32; margin:4px 8px; }

        #panel { background:transparent; }
        #panel QPushButton {
            background:transparent; border:0; padding:0;
            color:#D2D2D8; border-radius:8px;
        }
        #panel QPushButton:hover { background:rgba(255,255,255,20); }
        #panel QPushButton:pressed { background:rgba(255,255,255,10); }
        #panel QPushButton:checked { background:rgba(163,240,206,33); }
        #panel QPushButton:disabled { background:transparent; color:#5C5C64; }
        #panel QPushButton#mode { padding:0 11px; font-size:11.5px; color:#A6A6AE; }
        #panel QPushButton#mode:hover { color:#F4F4F5; }
        #panel QPushButton#modePrimary {
            padding:0 11px; font-size:11.5px; color:#F4F4F5;
            background:rgba(255,255,255,18);
        }
        #panel QPushButton#modePrimary:hover { background:rgba(255,255,255,31); }
        #panel QPushButton#modePrimary:disabled { background:transparent; color:#5C5C64; }
        #panel QPushButton#round { padding:0 9px; font-size:11px; color:#B9B9C1; border-radius:7px; }
        #panel QPushButton#round:checked { background:rgba(163,240,206,33); color:#A3F0CE; }
        #panel QLabel { background:transparent; }
        #panel QLabel#brand { font-size:12px; }
        #panel QLabel#status { font-size:11px; color:#C3C3CA; }
        #panel QLabel#size { font-size:10.5px; color:#8E8E97; }
        #panel QLabel#emptyHint { color:#5F5F68; font-size:10.5px; }
        #panel QPushButton#emptyAction {
            background:rgba(255,255,255,9); border:1px solid rgba(255,255,255,22);
            border-radius:17px; padding:0 14px; color:#C9C9CF; font-size:11.5px;
        }
        #panel QPushButton#emptyAction:hover {
            background:rgba(255,255,255,16); border-color:rgba(163,240,206,90);
            color:#F4F4F5;
        }
        #panel QLabel#paramLabel { font-size:10.5px; color:#9A9AA2; }
        #panel QLabel#paramValue { font-size:11px; color:#C3C3CA; }
        #panel QToolButton { background:transparent; border:0; border-radius:8px; }
        #panel QToolButton:hover { background:rgba(255,255,255,20); }
        #panel QToolButton::menu-indicator { image:none; width:0; }
        #panel QSlider::groove:horizontal { height:4px; background:rgba(255,255,255,31); border-radius:2px; }
        #panel QSlider::sub-page:horizontal { background:#A3F0CE; border-radius:2px; }
        #panel QSlider::handle:horizontal {
            width:12px; height:12px; margin:-4px 0;
            border-radius:6px; background:#E9FBF3;
        }
        #panel QSlider::handle:horizontal:hover { background:#FFFFFF; }

        #pill, #dock, #params, #toast {
            background:qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(38,38,45,229),
                stop:0.12 rgba(18,18,23,224),
                stop:1 rgba(9,9,12,230));
            border:1px solid rgba(255,255,255,26);
        }
        #pill { border-radius:17px; }
        #dock { border-radius:14px; }
        #params { border-radius:17px; }
        #toast { border-radius:99px; }
        #toast QLabel { background:transparent; color:#E9E9EC; font-size:11px; }

        #shotPopup { background:transparent; }
        #shotPopup QLabel { color:#8E8E97; font-size:10px; }
        QPushButton#shotThumb { background:rgba(255,255,255,6);
            border:1px solid rgba(255,255,255,16); border-radius:8px; padding:0; }
        QPushButton#shotThumb:hover { background:rgba(255,255,255,12);
            border-color:rgba(163,240,206,110); }

        #colorPopup { background:transparent; }
        #colorPopup QLabel { color:#8E8E97; font-size:10px; }
        #colorPopup QPushButton { background:transparent; border:0; border-radius:8px; padding:0; }
        #colorPopup QPushButton:hover { background:rgba(255,255,255,18); }
        #colorPopup QPushButton#popupMore {
            background:rgba(255,255,255,10);
            border:1px solid rgba(255,255,255,18);
            color:#C9C9CF; font-size:11px; padding:0 9px; text-align:left;
        }
        #colorPopup QPushButton#popupMore:hover { background:rgba(255,255,255,18); }
    )");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"background", "在后台启动"});
    parser.addOption({"settings", "打开设置"});
    parser.addOption({"capture", "开始截图：region、screen 或 window", "mode"});
    parser.process(app);
    if (!parser.positionalArguments().isEmpty() || parser.optionNames().size() > 1) return 2;
    QString command = "present";
    if (parser.isSet("background")) command = "background";
    if (parser.isSet("settings")) command = "settings";
    if (parser.isSet("capture")) {
        command = parser.value("capture");
        if (!QStringList{"region","screen","window"}.contains(command)) return 2;
    }
    Panel panel;
    QByteArray display = qgetenv("DISPLAY");
    if (display.endsWith(".0")) display.chop(2);
    const QByteArray session = display + '/' + qgetenv("WAYLAND_DISPLAY");
    const QString service = "org.lumenshot.Session.s" +
        QCryptographicHash::hash(session, QCryptographicHash::Sha256).toHex();
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerObject("/Panel", &panel, QDBusConnection::ExportScriptableSlots))
        return 1;
    if (!bus.registerService(service)) {
        QDBusInterface existing(service, "/Panel", "org.lumenshot.Panel", bus);
        return existing.call("Dispatch", command).type() == QDBusMessage::ErrorMessage ? 1 : 0;
    }
    panel.startDesktop();
    QTimer::singleShot(0, &panel, [&panel,command]{ panel.Dispatch(command); });
    return app.exec();
}

#include "panel.moc"
