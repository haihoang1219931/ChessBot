#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "assistant/AssistantController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    AssistantController assistant;
    assistant.setAIModel("Tobot",
                         "Player",
                         "ggml-base.en.bin",
                         "qwen2.5-1.5b-instruct-q4_k_m.gguf",
                         "piper",
                         "en_US-sam-medium.onnx");
    engine.rootContext()->setContextProperty("assistant", &assistant);
    engine.load(QUrl(QStringLiteral("qrc:/main_Assistant.qml")));
    if (engine.rootObjects().isEmpty())
            return -1;
    return app.exec();
}
