#include "preferences.h"
#include "hotkey.h"
#include <QtWidgets>
#include <QPainter>
#include <QScreen>
#include <QVariantAnimation>
#include <QWindow>

namespace {
QSettings prefs() { return QSettings(QSettings::IniFormat, QSettings::UserScope, "Lumenshot", "desktop"); }
QString startupFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/lumenshot.desktop";
}
bool changeStartup(bool on)
{
    QString path = startupFile();
    if (!on) return !QFileInfo::exists(path) || QFile::remove(path);
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QString launcher = QCoreApplication::applicationFilePath();
    const QFileInfo wrapper(QCoreApplication::applicationDirPath() + "/../start");
    if (wrapper.isExecutable() && wrapper.isFile()) launcher = wrapper.canonicalFilePath();
    launcher.replace('\\', "\\\\").replace('"', "\\\"").replace('$', "\\$").replace('`', "\\`").replace('%', "%%");
    launcher.replace('\\', "\\\\");
    const QByteArray entry = QString("[Desktop Entry]\nType=Application\nName=Lumenshot\nExec=\"%1\" --background\nTerminal=false\n").arg(launcher).toUtf8();
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(entry) == entry.size() && file.commit();
}

class Switch : public QAbstractButton {
public:
    explicit Switch(QWidget *parent = nullptr) : QAbstractButton(parent)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(38, 22);
        setFocusPolicy(Qt::NoFocus);
        animation.setDuration(140);
        animation.setEasingCurve(QEasingCurve::OutCubic);
        connect(&animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
            position = value.toReal();
            update();
        });
        connect(this, &QAbstractButton::toggled, this, [this](bool checked){
            animation.stop();
            animation.setStartValue(position);
            animation.setEndValue(checked ? 1.0 : 0.0);
            animation.start();
        });
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const qreal radius = height() / 2.0;
        const QColor off(255, 255, 255, 30), on(163, 240, 206);
        const QColor track(qRound(off.red() + (on.red() - off.red()) * position),
                           qRound(off.green() + (on.green() - off.green()) * position),
                           qRound(off.blue() + (on.blue() - off.blue()) * position),
                           qRound(off.alpha() + (on.alpha() - off.alpha()) * position));
        p.setPen(Qt::NoPen);
        p.setBrush(track);
        p.drawRoundedRect(QRectF(rect()), radius, radius);
        const qreal knob = height() - 6;
        const QColor knobColor = isChecked() ? QColor("#07120E") : QColor("#E9E9EC");
        p.setBrush(knobColor);
        p.drawEllipse(QRectF(3 + position * (width() - knob - 6), 3, knob, knob));
    }
    void checkStateSet() override
    {
        position = isChecked() ? 1.0 : 0.0;
        update();
    }
private:
    QVariantAnimation animation;
    qreal position = 0;
};

class PreferencesDialog : public QDialog {
public:
    PreferencesDialog(QWidget *parent, Hotkey &hotkey, bool &registered)
        : QDialog(parent), hotkey(hotkey), registered(registered)
    {
        setObjectName("prefs");
        setWindowTitle("Lumenshot · 设置");
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        setFixedWidth(480);
        build();
        adjustSize();
        if (parent) {
            const QPoint center = parent->window()->frameGeometry().center();
            move(center.x() - width() / 2, center.y() - height() / 2);
        } else if (QScreen *display = QGuiApplication::primaryScreen()) {
            const QPoint center = display->availableGeometry().center();
            move(center.x() - width() / 2, center.y() - height() / 2);
        }
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(QColor(255, 255, 255, 26), 1));
        p.setBrush(QColor("#0C0C0F"));
        p.drawRoundedRect(card, 16, 16);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && event->position().y() < 60 && windowHandle()) {
            windowHandle()->startSystemMove();
            return;
        }
        QDialog::mousePressEvent(event);
    }
private:
    Hotkey &hotkey;
    bool &registered;
    QLabel *state = nullptr;
    QLabel *dot = nullptr;
    QList<QPushButton *> segments;
    QList<QString> choices;

    static QLabel *section(const QString &text)
    {
        auto *label = new QLabel(text);
        label->setObjectName("section");
        return label;
    }

    void build()
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 22, 24, 22);
        layout->setSpacing(0);

        auto *header = new QHBoxLayout;
        auto *title = new QLabel("设置");
        title->setObjectName("prefsTitle");
        header->addWidget(title);
        header->addStretch();
        auto *close = new QPushButton("✕");
        close->setObjectName("close");
        close->setFixedSize(28, 28);
        close->setCursor(Qt::PointingHandCursor);
        close->setFocusPolicy(Qt::NoFocus);
        close->setAccessibleName("关闭设置");
        connect(close, &QPushButton::clicked, this, &QDialog::reject);
        header->addWidget(close);
        layout->addLayout(header);
        layout->addSpacing(18);

        layout->addWidget(section("区域截图附加快捷键（F1 始终可用）"));
        layout->addSpacing(8);
        auto *grid = new QGridLayout;
        grid->setSpacing(8);
        choices = {"Ctrl+Alt+A", "Ctrl+Alt+S", "Ctrl+Alt+F8", "off"};
        const QStringList labels = {"Ctrl + Alt + A", "Ctrl + Alt + S", "Ctrl + Alt + F8", "关闭快捷键"};
        auto *group = new QButtonGroup(this);
        group->setExclusive(true);
        for (int i = 0; i < labels.size(); ++i) {
            auto *button = new QPushButton(labels.at(i));
            button->setObjectName("seg");
            button->setCheckable(true);
            button->setFixedHeight(34);
            button->setCursor(Qt::PointingHandCursor);
            button->setAccessibleName(labels.at(i));
            group->addButton(button, i);
            segments.append(button);
            grid->addWidget(button, i / 2, i % 2);
        }
        layout->addLayout(grid);
        layout->addSpacing(9);

        auto *stateRow = new QHBoxLayout;
        stateRow->setSpacing(7);
        dot = new QLabel;
        dot->setFixedSize(6, 6);
        state = new QLabel;
        state->setObjectName("state");
        stateRow->addWidget(dot);
        stateRow->addWidget(state);
        stateRow->addStretch();
        layout->addLayout(stateRow);

        const bool x11 = qEnvironmentVariable("XDG_SESSION_TYPE") == "x11";
        for (auto *button : segments) button->setEnabled(x11);
        auto selected = [this]{
            const QString current = shortcutChoice();
            for (int i = 0; i < choices.size(); ++i)
                if (choices.at(i) == current) return i;
            return 0;
        };
        segments.at(selected())->setChecked(true);
        refreshState(x11);

        connect(group, &QButtonGroup::idClicked, this, [this, x11](int id){
            if (!x11) return;
            const QString choice = choices.at(id);
            if (choice == shortcutChoice() && registered) return;
            if (!hotkey.bind(choice)) {
                const QString before = shortcutChoice();
                for (int i = 0; i < choices.size(); ++i)
                    if (choices.at(i) == before) segments.at(i)->setChecked(true);
                state->setText("按键注册失败，请换一组按键");
                dot->setStyleSheet("background:#F0C97E;border-radius:3px;");
                return;
            }
            prefs().setValue("shortcut", choice);
            registered = choice != "off";
            refreshState(x11);
        });

        layout->addSpacing(18);
        auto *divider = new QWidget;
        divider->setFixedHeight(1);
        divider->setStyleSheet("background:rgba(255,255,255,14);");
        layout->addWidget(divider);
        layout->addSpacing(16);

        layout->addWidget(section("默认保存目录"));
        layout->addSpacing(8);
        auto *pathRow = new QHBoxLayout;
        pathRow->setSpacing(8);
        auto *path = new QLineEdit(saveDirectory());
        path->setObjectName("path");
        path->setReadOnly(true);
        path->setFixedHeight(34);
        path->setCursorPosition(0);
        path->setToolTip(path->text());
        path->setAccessibleName("默认保存目录");
        auto *change = new QPushButton("更改");
        change->setObjectName("ghost");
        change->setFixedSize(62, 34);
        change->setCursor(Qt::PointingHandCursor);
        change->setAccessibleName("更改保存目录");
        pathRow->addWidget(path, 1);
        pathRow->addWidget(change);
        layout->addLayout(pathRow);

        connect(change, &QPushButton::clicked, this, [this, path]{
            const QString folder = QFileDialog::getExistingDirectory(this, "选择保存目录", path->text());
            if (folder.isEmpty()) return;
            if (!QFileInfo(folder).isWritable()) {
                QMessageBox::warning(this, "目录不可写", "请选择可以写入的目录。");
                return;
            }
            prefs().setValue("saveDirectory", folder);
            path->setText(folder);
            path->setCursorPosition(0);
            path->setToolTip(folder);
        });

        layout->addSpacing(18);
        layout->addWidget(section("启动与托盘"));
        layout->addSpacing(10);

        auto *startupRow = new QHBoxLayout;
        auto *startupLabel = new QLabel("登录桌面后在后台启动");
        startupLabel->setObjectName("row");
        auto *startup = new Switch;
        startup->setChecked(QFileInfo::exists(startupFile()));
        startup->setAccessibleName("登录桌面后在后台启动");
        startupRow->addWidget(startupLabel);
        startupRow->addStretch();
        startupRow->addWidget(startup);
        layout->addLayout(startupRow);
        connect(startup, &QAbstractButton::toggled, this, [this, startup](bool checked){
            if (changeStartup(checked)) return;
            QSignalBlocker blocker(startup);
            startup->setChecked(!checked);
            QMessageBox::warning(this, "设置未保存", "请检查用户自启动目录的写入权限。");
        });

        layout->addSpacing(12);
        auto *compactRow = new QHBoxLayout;
        auto *compactLabel = new QLabel("紧凑布局（下次启动生效）");
        compactLabel->setObjectName("row");
        auto *compact = new Switch;
        compact->setChecked(prefs().value("compactUI", false).toBool());
        compact->setAccessibleName("紧凑布局");
        compactRow->addWidget(compactLabel);
        compactRow->addStretch();
        compactRow->addWidget(compact);
        layout->addLayout(compactRow);
        connect(compact, &QAbstractButton::toggled, this, [](bool checked){
            prefs().setValue("compactUI", checked);
        });

        layout->addSpacing(12);
        auto *trayRow = new QHBoxLayout;
        auto *trayLabel = new QLabel("系统托盘");
        trayLabel->setObjectName("row");
        auto *trayValue = new QLabel(QSystemTrayIcon::isSystemTrayAvailable() ? "可用" : "未检测到");
        trayValue->setObjectName("value");
        trayRow->addWidget(trayLabel);
        trayRow->addStretch();
        trayRow->addWidget(trayValue);
        layout->addLayout(trayRow);

        layout->addSpacing(22);
        auto *footer = new QHBoxLayout;
        auto *note = new QLabel("修改会立即生效");
        note->setObjectName("hint");
        auto *done = new QPushButton("完成");
        done->setObjectName("primary");
        done->setFixedSize(88, 36);
        done->setCursor(Qt::PointingHandCursor);
        done->setDefault(true);
        done->setAccessibleName("完成设置");
        connect(done, &QPushButton::clicked, this, &QDialog::accept);
        footer->addWidget(note);
        footer->addStretch();
        footer->addWidget(done);
        layout->addLayout(footer);

        setStyleSheet(R"(
            QDialog#prefs { background:transparent; }
            QLabel#prefsTitle { color:#F4F4F5; font-size:16px; font-weight:600; }
            QLabel#section { color:#8E8E97; font-size:10.5px; font-weight:600; }
            QLabel#state { color:#A6A6AE; font-size:11px; }
            QLabel#row { color:#E4E4E8; font-size:12.5px; }
            QLabel#value { color:#9A9AA2; font-size:11.5px; }
            QLabel#hint { color:#6F6F78; font-size:11px; }
            QPushButton#close { background:transparent; border:0; border-radius:8px;
                color:#A6A6AE; font-size:14px; padding:0; }
            QPushButton#close:hover { background:rgba(255,69,58,62); color:#FFFFFF; }
            QPushButton#seg { background:rgba(255,255,255,7); border:1px solid rgba(255,255,255,18);
                border-radius:9px; color:#B9B9C1; font-size:11.5px; }
            QPushButton#seg:hover { background:rgba(255,255,255,14); color:#F4F4F5; }
            QPushButton#seg:checked { background:rgba(163,240,206,30);
                border:1px solid rgba(163,240,206,150); color:#DFFCEF; }
            QPushButton#seg:disabled { color:#5C5C64; }
            QPushButton#ghost { background:rgba(255,255,255,10); border:1px solid rgba(255,255,255,18);
                border-radius:8px; color:#D2D2D8; font-size:11.5px; }
            QPushButton#ghost:hover { background:rgba(255,255,255,18); }
            QPushButton#primary { background:#A3F0CE; border:0; border-radius:9px;
                color:#07120E; font-size:12px; font-weight:600; }
            QPushButton#primary:hover { background:#B8F7DC; }
            QPushButton#primary:pressed { background:#8FE3BE; }
            QLineEdit#path { background:rgba(255,255,255,7); border:1px solid rgba(255,255,255,18);
                border-radius:8px; color:#C9C9CF; font-size:11.5px; padding:0 10px; }
            QLineEdit#path:focus { border-color:rgba(163,240,206,90); }
        )");
    }

    void refreshState(bool x11)
    {
        QString text;
        QString color = "#6F6F78";
        if (!x11) {
            text = "Wayland 截图待验收";
        } else if (shortcutChoice() == "off") {
            text = "快捷键已关闭";
        } else if (registered) {
            text = "快捷键已注册";
            color = "#A3F0CE";
        } else {
            text = "快捷键不可用，请换一组按键";
            color = "#F0C97E";
        }
        state->setText(text);
        dot->setStyleSheet(QString("background:%1;border-radius:3px;").arg(color));
    }
};
}

QString saveDirectory()
{
    return prefs().value("saveDirectory", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString();
}
QString shortcutChoice() { return prefs().value("shortcut", "Ctrl+Alt+A").toString(); }

void showPreferences(QWidget *parent, Hotkey &hotkey, bool &registered)
{
    PreferencesDialog dialog(parent, hotkey, registered);
    dialog.exec();
}
