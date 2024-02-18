#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QIcon>
#include "Converter.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/Mebo_MK3_Converter/Main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    app.setWindowIcon(QIcon(":/Logo_orange.svg"));

    Converter *conv = new Converter();
    engine.rootContext()->setContextProperty("conv", conv);

    engine.load(url);

    return app.exec();
}
