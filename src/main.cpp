#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>
#include <QUrl>

#include <cstdio>

#include "store/Database.h"
#include "app/ItemModel.h"
#include "app/TodayModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("open-woday"));

    // The local database is opened before any window is created. If it cannot
    // be created or opened, startup fails loudly with a non-zero exit code
    // rather than leaving a silently half-working planner.
    Database database;
    if (!database.open()) {
        std::fprintf(stderr,
                     "open-woday: startup failed - the local database could "
                     "not be opened: %s; no window was opened.\n",
                     qPrintable(database.lastError()));
        return 1;
    }

    TodayModel todayModel(database);
    ItemModel itemModel(database);
    QObject::connect(&itemModel, &ItemModel::taskMutated,
                     &todayModel, &TodayModel::refresh);
    QObject::connect(&todayModel, &TodayModel::taskUpdated,
                     &itemModel, &ItemModel::syncTask);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("todayModel"),
                                             &todayModel);
    engine.rootContext()->setContextProperty(QStringLiteral("itemModel"),
                                             &itemModel);
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
