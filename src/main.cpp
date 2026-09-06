#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ArenaMP"));
    QCoreApplication::setApplicationName(QStringLiteral("ArenaDDSOptimizer"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    MainWindow w;
    w.show();
    return app.exec();
}
