#include <QApplication>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "backend/controllers/app_controller.h"
int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QApplication app(argc, argv);
    QCoreApplication::addLibraryPath(QLibraryInfo::path(QLibraryInfo::PluginsPath));
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());

    QQmlApplicationEngine engine;
    AppController appController;
    engine.rootContext()->setContextProperty("appController", &appController);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("EdgeGuard", "Main");

    return app.exec();
}
