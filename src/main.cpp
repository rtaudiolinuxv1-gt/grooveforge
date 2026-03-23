#include <QApplication>

#include "app/GrooveController.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    groove::GrooveController controller;
    controller.initialize();

    groove::MainWindow window(&controller);
    window.show();

    return app.exec();
}
