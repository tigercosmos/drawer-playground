#include "MainWindow.h"

#include <QGuiApplication>
#include <QScreen>

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    if (QGuiApplication::screens().isEmpty()) {
        qCritical("No screens available for Qt Cocoa platform. Run from a desktop login session.");
        return 1;
    }

    MainWindow window;
    window.show();
    return app.exec();
}
