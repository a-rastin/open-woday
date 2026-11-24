#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QUrl>

#include <cstdio>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("open-woday"));

    QQmlApplicationEngine engine;
    const QUrl rootUrl(QStringLiteral("qrc:/qml/MainWindow.qml"));
    engine.load(rootUrl);

    if (engine.rootObjects().isEmpty()) {
        // Write straight to stderr: Qt's logging can be routed away from it
        // when the process has no terminal, which would hide this failure.
        std::fprintf(stderr,
                     "open-woday: startup failed - the root QML component "
                     "could not be created; no window was opened.\n");
        return 1;
    }

    return app.exec();
}
