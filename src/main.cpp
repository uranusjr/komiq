#include <QApplication>
#include <QCommandLineParser>
#include "centralwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Komiq");

    QCommandLineParser parser;
    parser.addPositionalArgument("files", "file to open", "[file ...]");
    parser.process(a);

    QStringList paths = parser.positionalArguments();

    CentralWidget w;
    w.showMaximized();

    if (paths.size() > 0)
    {
        QMetaObject::invokeMethod(&w, [&w, &paths]() {
            w.openLocalPaths(paths);
        }, Qt::QueuedConnection);
    }

    return a.exec();
}
