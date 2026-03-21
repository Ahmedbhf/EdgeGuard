#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "backend/datamodel.h"
int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;
    DataModel model;
    engine.rootContext()->setContextProperty("dataModel", &model);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("EdgeGuard", "Main");

    return app.exec();
}
