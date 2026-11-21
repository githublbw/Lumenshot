#include "hotkey.h"
#include <QCoreApplication>
#include <cstdio>

// Run inside the isolated X11 test after the panel owns Ctrl+Alt+A.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Hotkey first, second;
    if (first.bind("Ctrl+Alt+A") || !first.bind("Ctrl+Alt+S")) return 1;
    if (first.bind("Ctrl+Alt+A") || second.bind("Ctrl+Alt+S")) return 2;
    if (!first.bind("off") || !second.bind("Ctrl+Alt+S")) return 3;
    puts("Shortcut conflict, rollback and release passed");
}
