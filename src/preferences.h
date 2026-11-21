#pragma once
#include <QString>
class QWidget;
class Hotkey;

QString saveDirectory();
QString shortcutChoice();
void showPreferences(QWidget *parent, Hotkey &hotkey, bool &registered);
