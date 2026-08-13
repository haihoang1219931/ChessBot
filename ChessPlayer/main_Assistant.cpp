#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "assistant/AssistantController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    qmlRegisterType<AssistantController>("ChessAssistant", 1, 0, "AssistantController");

    engine.load(QUrl(QStringLiteral("qrc:/main_Assistant.qml")));
    if (engine.rootObjects().isEmpty())
            return -1;
    return app.exec();
}
