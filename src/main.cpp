#ifdef _WIN32

#include <QApplication>

#include "tray_application.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    arrower::TrayApplication tray_application;
    tray_application.Initialize();

    return app.exec();
}

#else

int main() {
    return 1;
}

#endif
