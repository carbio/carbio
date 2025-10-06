#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "controller.h"
#include "radialbar.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    qmlRegisterType<RadialBar>("CustomControls", 1, 0, "RadialBar");

    Controller controller;
    if (!controller.initializeSensor()) {
        qWarning() << "Sensor initialization failed - running in demo mode";
        // Controller should still work with default values
    }
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url] (QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        } 
    }, Qt::QueuedConnection);
    engine.load(url);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML";
        return -1;
    }
    return app.exec();
}
