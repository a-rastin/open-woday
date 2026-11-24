// The single high-level persistence seam of Phase 2 (issue 02, "Create and
// read an Item across an app restart"). One Task Item is written through the
// store layer, the Database connection is closed and reopened against the same
// local SQLite file, and the Item is read back by id through the same public
// API.
//
// This is an executable specification, not a test framework: no QtTest, no
// catch/gtest, no CI configuration, no mocks, no assertions about SQL strings
// or private helpers. It observes only externally visible Item values.

#include <QCoreApplication>
#include <QString>
#include <QTemporaryDir>
#include <QtGlobal>

#include <cstdio>
#include <optional>

#include "store/Database.h"
#include "store/ItemRepository.h"

namespace {

int fail(const QString& reason)
{
    std::fprintf(stderr, "verify-persistence: FAIL - %s\n",
                 qPrintable(reason));
    return 1;
}

bool check(bool condition, const QString& reason)
{
    if (!condition) {
        fail(reason);
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("open-woday"));

    // Work in a fresh app-data location so the round trip is independent of any
    // planner data already on this device.
    QTemporaryDir dataDir;
    if (!dataDir.isValid()) {
        return fail(QStringLiteral(
            "a fresh app-data location could not be created"));
    }
    qputenv("XDG_DATA_HOME", dataDir.path().toUtf8());
    if (!Database::defaultDataDirectory().startsWith(dataDir.path())) {
        return fail(QStringLiteral(
            "the fresh app-data location was not used; refusing to touch the "
            "planner's data"));
    }

    const QString kind = QStringLiteral("task");
    const QString title = QStringLiteral("Reply to Anna");

    Item written;
    // First session: create one Task Item, then close the connection. The
    // Database destructor closes it: the app restart.
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);
        const std::optional<Item> created = items.create(kind, title);
        if (!created) {
            return fail(QStringLiteral(
                            "the Task Item could not be created: %1")
                            .arg(items.lastError()));
        }
        written = *created;
    }

    if (!check(written.id > 0,
               QStringLiteral("the created Item has no assigned id"))) {
        return 1;
    }

    // Second session: reopen against the same file and read the Item back.
    Item readBack;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);
        const std::optional<Item> read = items.readById(written.id);
        if (!read) {
            return fail(QStringLiteral(
                            "the Item was not readable after the restart: %1")
                            .arg(items.lastError()));
        }
        readBack = *read;
    }

    if (!check(readBack.id == written.id,
               QStringLiteral("the Item id changed across the restart"))) {
        return 1;
    }
    if (!check(readBack.kind == kind,
               QStringLiteral("the Item kind changed across the restart"))) {
        return 1;
    }
    if (!check(readBack.title == title,
               QStringLiteral("the Item title changed across the restart"))) {
        return 1;
    }
    if (!check(readBack.createdAt == written.createdAt,
               QStringLiteral("created_at changed across the restart"))) {
        return 1;
    }
    if (!check(readBack.modifiedAt == written.modifiedAt,
               QStringLiteral("modified_at changed across the restart"))) {
        return 1;
    }
    if (!check(!readBack.createdAt.isEmpty() && !readBack.modifiedAt.isEmpty(),
               QStringLiteral("the Item timestamps are empty"))) {
        return 1;
    }

    std::puts("verify-persistence: PASS - a Task Item written before a restart "
              "reads back with its fields unchanged");
    return 0;
}
