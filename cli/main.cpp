// Disable strict-overflow for Qt headers only (Qt's internal templates trigger false positives)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "controller.h"
#include "radialbar.h"

#include <spdlog/spdlog.h>


int main(int argc, char *argv[])
{
  spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  spdlog::set_level(spdlog::level::trace);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    qmlRegisterType<RadialBar>("CustomControls", 1, 0, "RadialBar");

    Controller controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url, &controller] (QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        } else if (obj && url == objUrl) {
            // Initialize sensor AFTER QML loads successfully
            if (!controller.initializeSensor()) {
                // qWarning() << "Sensor initialization failed - running in demo mode";
            }
        }
    }, Qt::QueuedConnection);
    engine.load(url);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML";
        return -1;
    }
    return app.exec();
}
