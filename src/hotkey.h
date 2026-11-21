#pragma once
#include <QObject>
#include <memory>

class Hotkey : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~Hotkey();
    bool bind(const QString &choice);
signals:
    void triggered();
private:
    struct Binding;
    Binding *binding = nullptr;
};
