#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication app(argc, argv);
    MainWindow w;
    w.setWindowTitle("Анализ расхода молочного сырья");

    w.show();
    return app.exec();
}
