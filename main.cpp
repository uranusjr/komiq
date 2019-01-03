#include <QApplication>
#include "centralwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CentralWidget w;
    w.showMaximized();
    return a.exec();
}
