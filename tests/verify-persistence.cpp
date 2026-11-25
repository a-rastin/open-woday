// The single high-level persistence seam of Phase 2 (issues 02, 03, 04 and 05):
// Items and Blocks are written through the store layer, the Database connection
// is closed and reopened against the same local SQLite file, and everything is
// read back through the same public API — with every column, kind, query shape,
// update, soft-delete, Block order and constraint error intact. Issue 05 adds the
// merge-gate scenario: one representative set of Items of all kinds and Blocks
// of all five types, observed with the first-launch schema version and the
// persisted meta version through Database::schemaVersion().
//
// This is an executable specification, not a test framework: no QtTest, no
// catch/gtest, no CI configuration, no mocks, no assertions about SQL strings
// or private helpers. It observes only externally visible Item and Block
// values through the repositories. Folders and Tags have no repository in this
// phase, so their few fixture rows are seeded with bound values; the one raw
// DELETE that triggers the Block cascade is fixture setup too. No assertion
// reads persisted state through a side channel.

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QVariant>
#include <QtGlobal>

#include <cstdio>
#include <optional>

#include "store/Database.h"
#include "store/BlockRepository.h"
#include "store/ItemRepository.h"
#include "app/TodayModel.h"

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

// Point the app at a fresh app-data location before each scenario, so every
// round trip starts from a first launch and no planner data on this device is
// touched. QStandardPaths reads the environment on each call.
int freshAppData(QTemporaryDir* dataDir)
{
    if (!dataDir->isValid()) {
        return fail(QStringLiteral(
            "a fresh app-data location could not be created"));
    }
    qputenv("XDG_DATA_HOME", dataDir->path().toUtf8());
    if (!Database::defaultDataDirectory().startsWith(dataDir->path())) {
        return fail(QStringLiteral(
            "the fresh app-data location was not used; refusing to touch the "
            "planner's data"));
    }
    return 0;
}

} // namespace

// Exercise the Phase 4 restart path as one scenario, including date changes
// and an Event created through the existing store API.
int todayDataSurvivesRestartTogether()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    const QString tomorrow = QDate::currentDate().addDays(1).toString(Qt::ISODate);
    const QString writing = QString::fromUtf8("Plan ahead\n\nسلام");
    qlonglong noteId = 0;
    qlonglong eventId = 0;
    QString taskId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        ItemRepository items(database);
        Item event;
        event.kind = QStringLiteral("event");
        event.title = QStringLiteral("Design review");
        event.startAt = today + QStringLiteral("T10:00:00");
        event.endAt = today + QStringLiteral("T11:00:00");
        const auto createdEvent = items.create(event);
        if (!createdEvent)
            return fail(items.lastError());
        eventId = createdEvent->id;

        TodayModel model(database);
        model.setDayNoteText(writing);
        model.addTask(QStringLiteral("Pack lunch"));
        if (!check(model.lines().size() == 2
                       && model.lines().first().toMap().value(QStringLiteral("kind"))
                           == QStringLiteral("task"),
                   QStringLiteral("Task and Event lines missing")))
            return 1;
        taskId = model.lines().first().toMap().value(QStringLiteral("id")).toString();
        if (!check(model.setTaskCompleted(taskId, true),
                   QStringLiteral("Task could not be completed")))
            return 1;
        const auto notes = items.readByDay(today);
        if (!check(notes.size() == 1 && notes.first().kind == QStringLiteral("note"),
                   QStringLiteral("opening today did not create one Day note")))
            return 1;
        noteId = notes.first().id;
        model.selectDate(tomorrow);
        if (!check(items.readByDay(tomorrow).size() == 1 && model.dayNoteText().isEmpty()
                       && model.lines().isEmpty(),
                   QStringLiteral("opening another date did not create its Day note")))
            return 1;
        model.selectDate(today);
        if (!check(model.dayNoteText() == writing && items.readByDay(today).size() == 1
                       && items.readByDay(today).first().id == noteId,
                   QStringLiteral("reopening today did not reuse its Day note")))
            return 1;
    }
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        TodayModel model(database);
        ItemRepository items(database);
        BlockRepository blocks(database);
        const auto notes = items.readByDay(today);
        const auto event = items.readById(eventId);
        const auto task = items.readById(taskId.toLongLong());
        const auto storedBlocks = blocks.readByItem(noteId);
        if (!check(notes.size() == 1 && notes.first().id == noteId
                       && model.dayNoteText() == writing
                       && storedBlocks.size() == 3
                       && storedBlocks.at(0).content == QStringLiteral("Plan ahead")
                       && storedBlocks.at(1).content.isEmpty()
                       && storedBlocks.at(2).content == QString::fromUtf8("سلام")
                       && storedBlocks.at(2).position == 2,
                   QStringLiteral("Day note or ordered writing changed after restart")))
            return 1;
        bool completedTaskVisible = false;
        bool eventVisible = false;
        for (const QVariant& line : model.lines()) {
            const QVariantMap values = line.toMap();
            if (values.value(QStringLiteral("id")).toString() == taskId
                && values.value(QStringLiteral("kind")) == QStringLiteral("task")
                && values.value(QStringLiteral("title")) == QStringLiteral("Pack lunch")
                && values.value(QStringLiteral("completed")).toBool())
                completedTaskVisible = true;
            if (values.value(QStringLiteral("id")).toString() == QString::number(eventId)
                && values.value(QStringLiteral("kind")) == QStringLiteral("event")
                && values.value(QStringLiteral("title")) == QStringLiteral("Design review")
                && !values.contains(QStringLiteral("completed")))
                eventVisible = true;
        }
        if (!check(model.lines().size() == 2 && task && task->completedAt && completedTaskVisible
                       && eventVisible
                       && event && event->title == QStringLiteral("Design review")
                       && event->startAt == today + QStringLiteral("T10:00:00")
                       && event->endAt == today + QStringLiteral("T11:00:00")
                       && model.summary() == QStringLiteral("1 task and 1 event on this day"),
                   QStringLiteral("Task, Event or summary changed after restart")))
            return 1;
        model.selectDate(tomorrow);
        if (!check(items.readByDay(tomorrow).size() == 1 && model.dayNoteText().isEmpty()
                       && model.lines().isEmpty(),
                   QStringLiteral("the other Day note was not reused after restart")))
            return 1;
    }
    return 0;
}

int todayTaskWriteSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        TodayModel model(database);
        if (!check(QMetaObject::invokeMethod(&model, "addTask",
                                              Q_ARG(QString, QStringLiteral("Pack lunch"))),
                   QStringLiteral("Task creation is unavailable")))
            return 1;
    }
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        TodayModel model(database);
        const QVariantList lines = model.property("lines").toList();
        if (!check(lines.size() == 1
                       && lines.first().toMap().value("title").toString()
                           == QStringLiteral("Pack lunch"),
                   QStringLiteral("Task line did not survive restart")))
            return 1;
    }
    return 0;
}

int todayCompletionSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    QString id;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        TodayModel model(database);
        model.addTask(QStringLiteral("Pack lunch"));
        if (!check(model.lines().size() == 1 && model.taskCount() == 1,
                   QStringLiteral("the selected day did not show its Task")))
            return 1;
        id = model.lines().first().toMap().value("id").toString();
        if (!check(model.setTaskCompleted(id, true)
                       && model.lines().first().toMap().value("completed").toBool()
                       && model.summary() == QStringLiteral("1 task and 0 events on this day"),
                   QStringLiteral("completion or summary did not update")))
            return 1;
        model.selectDate(QDate::currentDate().addDays(1).toString(Qt::ISODate));
        if (!check(model.lines().isEmpty() && model.taskCount() == 0
                       && !model.setTaskCompleted(id, false),
                   QStringLiteral("another day's Task was exposed or changed")))
            return 1;
    }
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        TodayModel model(database);
        ItemRepository items(database);
        const auto stored = items.readById(id.toLongLong());
        if (!check(stored && stored->completedAt
                       && QDateTime::fromString(*stored->completedAt, Qt::ISODate).isValid()
                       && model.lines().size() == 1
                       && model.lines().first().toMap().value("completed").toBool(),
                   QStringLiteral("Task completion did not survive restart")))
            return 1;
        if (!check(model.setTaskCompleted(id, false)
                       && !model.lines().first().toMap().value("completed").toBool(),
                   QStringLiteral("Task completion could not be cleared")))
            return 1;
    }
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());
        ItemRepository items(database);
        const auto stored = items.readById(id.toLongLong());
        if (!check(stored && !stored->completedAt,
                   QStringLiteral("cleared completion did not survive restart")))
            return 1;
    }
    return 0;
}

int todayCompletionRefreshesAlreadyStoredState()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    TodayModel model(database);
    model.addTask(QStringLiteral("Pack lunch"));
    const QString id = model.lines().first().toMap().value("id").toString();
    ItemRepository items(database);
    auto task = items.readById(id.toLongLong());
    if (!task)
        return fail(items.lastError());
    task->completedAt = QStringLiteral("2026-11-25T10:00:00Z");
    if (!items.update(*task))
        return fail(items.lastError());
    if (!check(model.setTaskCompleted(id, true)
                   && model.lines().first().toMap().value("completed").toBool(),
               QStringLiteral("already-stored completion was not reflected in the line")))
        return 1;
    return 0;
}

int richNoteEditIsSafe()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    ItemRepository items(database);
    Item note;
    note.kind = QStringLiteral("note");
    note.day = QDate::currentDate().toString(Qt::ISODate);
    const auto storedNote = items.create(note);
    if (!storedNote)
        return fail(items.lastError());
    BlockRepository blocks(database);
    Block heading;
    heading.itemId = storedNote->id;
    heading.type = QStringLiteral("heading");
    heading.content = QStringLiteral("Original");
    const auto stored = blocks.create(heading);
    if (!stored)
        return fail(blocks.lastError());
    TodayModel model(database);
    model.setDayNoteText(QStringLiteral("Changed"));
    model.selectDate(QDate::currentDate().addDays(1).toString(Qt::ISODate));
    const auto after = blocks.readById(stored->id);
    if (!check(after && after->type == heading.type && after->content == heading.content
                   && model.selectedDate() == *note.day
                   && model.dayNoteText() == QStringLiteral("Changed")
                   && !model.dayNoteError().isEmpty(),
               QStringLiteral("rich note or failed edit was lost")))
        return 1;
    return 0;
}

int dayNoteWritingSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }
    const QString writing = QString::fromUtf8("First line\n\nسلام\nLast line");
    QString day;
    {
        Database database;
        if (!database.open()) {
            return fail(database.lastError());
        }
        TodayModel model(database);
        day = model.selectedDate();
        if (!check(QMetaObject::invokeMethod(&model, "setDayNoteText",
                                              Q_ARG(QString, writing)),
                   QStringLiteral("Day note editing is unavailable"))) {
            return 1;
        }
    }
    {
        Database database;
        if (!database.open()) {
            return fail(database.lastError());
        }
        TodayModel model(database);
        if (!check(model.property("dayNoteText").toString() == writing,
                   QStringLiteral("Day note writing did not survive restart"))) {
            return 1;
        }
        ItemRepository items(database);
        const QList<Item> notes = items.readByDay(day);
        if (!check(notes.size() == 1 && notes.first().kind == QStringLiteral("note"),
                   QStringLiteral("opening a date did not reuse one Day note"))) {
            return 1;
        }
        BlockRepository blocks(database);
        const QList<Block> lines = blocks.readByItem(notes.first().id);
        if (!check(lines.size() == 4 && lines.at(1).content.isEmpty()
                       && lines.at(2).content == QString::fromUtf8("سلام")
                       && lines.at(3).position == 3,
                   QStringLiteral("writing was not stored as ordered Blocks"))) {
            return 1;
        }
    }
    return 0;
}

// Issue 03, slice 1: every column of one Item round-trips unchanged through
// the write -> restart -> read-back seam. The expected values are independent
// literals (and explicit nulls), never values recomputed by the store.
int fullItemColumnsSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString kind = QStringLiteral("task");
    const QString title = QStringLiteral("Write the quarterly report");
    const QString createdAt = QStringLiteral("2026-11-20T08:15:00Z");
    const QString modifiedAt = QStringLiteral("2026-11-21T10:45:00Z");
    const std::optional<QString> completedAt =
        QStringLiteral("2026-11-22T16:00:00Z");
    const std::optional<QString> dueAt =
        QStringLiteral("2026-12-01T09:00:00Z");
    const std::optional<QString> duePrecision = QStringLiteral("time");
    const std::optional<QString> startAt =
        QStringLiteral("2026-11-30T14:00:00Z");
    const std::optional<QString> endAt =
        QStringLiteral("2026-11-30T15:30:00Z");
    const std::optional<int> durationMin = 90;
    const int priority = 3;
    const int sortOrder = 7;

    Item written;
    // First session: write one Item with every column set, then close the
    // connection (the app restart).
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item item;
        item.kind = kind;
        item.title = title;
        item.createdAt = createdAt;
        item.modifiedAt = modifiedAt;
        item.completedAt = completedAt;
        item.dueAt = dueAt;
        item.duePrecision = duePrecision;
        item.startAt = startAt;
        item.endAt = endAt;
        item.allDay = false;
        item.priority = priority;
        item.durationMin = durationMin;
        item.day = std::nullopt;
        item.parentId = std::nullopt;
        item.folderId = std::nullopt;
        item.deletedAt = std::nullopt;
        item.sortOrder = sortOrder;

        const std::optional<Item> created = items.create(item);
        if (!created) {
            return fail(QStringLiteral("the Item could not be created: %1")
                            .arg(items.lastError()));
        }
        written = *created;
    }

    if (!check(written.id > 0,
               QStringLiteral("the created Item has no assigned id"))) {
        return 1;
    }

    // Second session: reopen the same file and read the Item back.
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
               QStringLiteral("kind did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.title == title,
               QStringLiteral("title did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.createdAt == createdAt,
               QStringLiteral("created_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.modifiedAt == modifiedAt,
               QStringLiteral("modified_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.completedAt == completedAt,
               QStringLiteral("completed_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.deletedAt == std::nullopt,
               QStringLiteral("deleted_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.dueAt == dueAt,
               QStringLiteral("due_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.duePrecision == duePrecision,
               QStringLiteral("due_precision did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.startAt == startAt,
               QStringLiteral("start_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.endAt == endAt,
               QStringLiteral("end_at did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.allDay == false,
               QStringLiteral("all_day did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.priority == priority,
               QStringLiteral("priority did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.durationMin == durationMin,
               QStringLiteral("duration_min did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.parentId == std::nullopt,
               QStringLiteral("parent_id did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.folderId == std::nullopt,
               QStringLiteral("folder_id did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.day == std::nullopt,
               QStringLiteral("day did not round-trip unchanged"))) {
        return 1;
    }
    if (!check(readBack.sortOrder == sortOrder,
               QStringLiteral("sort_order did not round-trip unchanged"))) {
        return 1;
    }

    return 0;
}

bool containsId(const QList<Item>& items, qlonglong id)
{
    for (const Item& item : items) {
        if (item.id == id) {
            return true;
        }
    }
    return false;
}

// Issue 03, slice 2: Items of all three kinds (Task, Note, Event) survive a
// restart and are readable by the query shapes later phases need — kind, day
// and parent. A Task's parent_id is a subtask; a Day note carries its date.
int itemsOfAllKindsAndQueriesSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString taskKind = QStringLiteral("task");
    const QString noteKind = QStringLiteral("note");
    const QString eventKind = QStringLiteral("event");
    const QString taskTitle = QStringLiteral("Write the quarterly report");
    const QString subtaskTitle = QStringLiteral("Collect figures");
    const QString noteTitle = QStringLiteral("Tuesday");
    const QString eventTitle = QStringLiteral("Design review");
    const std::optional<QString> day = QStringLiteral("2026-11-24");
    const std::optional<QString> eventStartAt =
        QStringLiteral("2026-11-25T10:00:00Z");
    const std::optional<QString> eventEndAt =
        QStringLiteral("2026-11-25T11:00:00Z");
    const std::optional<int> eventDurationMin = 60;

    qlonglong taskId = 0;
    qlonglong subtaskId = 0;
    qlonglong noteId = 0;
    qlonglong eventId = 0;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item task;
        task.kind = taskKind;
        task.title = taskTitle;
        task.sortOrder = 1;
        const std::optional<Item> createdTask = items.create(task);
        if (!createdTask) {
            return fail(QStringLiteral("the Task could not be created: %1")
                            .arg(items.lastError()));
        }
        taskId = createdTask->id;

        Item subtask;
        subtask.kind = taskKind;
        subtask.title = subtaskTitle;
        subtask.parentId = taskId;
        subtask.sortOrder = 2;
        const std::optional<Item> createdSubtask = items.create(subtask);
        if (!createdSubtask) {
            return fail(QStringLiteral("the subtask could not be created: %1")
                            .arg(items.lastError()));
        }
        subtaskId = createdSubtask->id;

        Item note;
        note.kind = noteKind;
        note.title = noteTitle;
        note.day = day;
        note.sortOrder = 3;
        const std::optional<Item> createdNote = items.create(note);
        if (!createdNote) {
            return fail(QStringLiteral("the Day note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteId = createdNote->id;

        Item event;
        event.kind = eventKind;
        event.title = eventTitle;
        event.startAt = eventStartAt;
        event.endAt = eventEndAt;
        event.allDay = true;
        event.priority = 1;
        event.durationMin = eventDurationMin;
        event.sortOrder = 4;
        const std::optional<Item> createdEvent = items.create(event);
        if (!createdEvent) {
            return fail(QStringLiteral("the Event could not be created: %1")
                            .arg(items.lastError()));
        }
        eventId = createdEvent->id;
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const QList<Item> tasks = items.readByKind(taskKind);
        if (!check(tasks.size() == 2 && containsId(tasks, taskId)
                       && containsId(tasks, subtaskId),
                   QStringLiteral("reading by kind did not return both Tasks"))) {
            return 1;
        }

        const QList<Item> notes = items.readByKind(noteKind);
        if (!check(notes.size() == 1 && notes.first().id == noteId
                       && notes.first().day == day,
                   QStringLiteral(
                       "the Day note did not read back by kind with its day"))) {
            return 1;
        }

        const QList<Item> events = items.readByKind(eventKind);
        if (!check(events.size() == 1 && events.first().id == eventId
                       && events.first().allDay == true
                       && events.first().startAt == eventStartAt
                       && events.first().endAt == eventEndAt
                       && events.first().durationMin == eventDurationMin
                       && events.first().priority == 1,
                   QStringLiteral(
                       "the Event did not read back by kind with its fields"))) {
            return 1;
        }

        const QList<Item> byDay = items.readByDay(*day);
        if (!check(byDay.size() == 1 && byDay.first().id == noteId
                       && byDay.first().kind == noteKind,
                   QStringLiteral("reading by day did not return the Day note"))) {
            return 1;
        }

        const QList<Item> subtasks = items.readByParent(taskId);
        if (!check(subtasks.size() == 1 && subtasks.first().id == subtaskId
                       && subtasks.first().parentId
                           == std::optional<qlonglong>(taskId),
                   QStringLiteral(
                       "reading by parent did not return the subtask"))) {
            return 1;
        }

        const std::optional<Item> readTask = items.readById(taskId);
        if (!check(readTask && readTask->parentId == std::nullopt,
                   QStringLiteral("the parent Task's parent_id changed"))) {
            return 1;
        }
    }

    return 0;
}

// Folders and Tags have no repository in this phase (their product surfaces
// belong to a later phase). Seed the few rows that the folder/tag read shapes
// join against, with bound values, so those queries can be observed through
// the ItemRepository interface. This is fixture setup at the same persistence
// seam, not a separate seam: every assertion below reads through
// ItemRepository.
int seedFolderAndTag(QSqlDatabase connection, qlonglong folderId,
                     qlonglong tagId)
{
    QSqlQuery folder(connection);
    folder.prepare(QStringLiteral(
        "INSERT INTO folders (id, name) VALUES (?, ?)"));
    folder.addBindValue(folderId);
    folder.addBindValue(QStringLiteral("Work"));
    if (!folder.exec()) {
        return fail(QStringLiteral("the Folder fixture could not be written: %1")
                        .arg(folder.lastError().text()));
    }

    QSqlQuery tag(connection);
    tag.prepare(QStringLiteral("INSERT INTO tags (id, name) VALUES (?, ?)"));
    tag.addBindValue(tagId);
    tag.addBindValue(QStringLiteral("focus"));
    if (!tag.exec()) {
        return fail(QStringLiteral("the Tag fixture could not be written: %1")
                        .arg(tag.lastError().text()));
    }
    return 0;
}

// Issue 03, slice 3: Items are readable by Folder and by Tag across a restart,
// and folder_id round-trips with a real Folder row. Plain CRUD only: no Inbox
// grouping, Today page or Task detail pane.
int folderAndTagQueriesSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const qlonglong folderId = 41;
    const qlonglong tagId = 42;
    qlonglong filedId = 0;
    qlonglong taggedId = 0;
    qlonglong unfiledId = 0;

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        if (seedFolderAndTag(database.connection(), folderId, tagId) != 0) {
            return 1;
        }
        ItemRepository items(database);

        Item filed;
        filed.kind = QStringLiteral("task");
        filed.title = QStringLiteral("File the report");
        filed.folderId = folderId;
        const std::optional<Item> createdFiled = items.create(filed);
        if (!createdFiled) {
            return fail(QStringLiteral("the filed Item could not be created: %1")
                            .arg(items.lastError()));
        }
        filedId = createdFiled->id;

        Item tagged;
        tagged.kind = QStringLiteral("note");
        tagged.title = QStringLiteral("Reading list");
        tagged.folderId = folderId;
        const std::optional<Item> createdTagged = items.create(tagged);
        if (!createdTagged) {
            return fail(QStringLiteral("the tagged Item could not be created: %1")
                            .arg(items.lastError()));
        }
        taggedId = createdTagged->id;

        QSqlQuery link(database.connection());
        link.prepare(QStringLiteral(
            "INSERT INTO item_tags (item_id, tag_id) VALUES (?, ?)"));
        link.addBindValue(taggedId);
        link.addBindValue(tagId);
        if (!link.exec()) {
            return fail(QStringLiteral(
                            "the Tag could not be attached to the Item: %1")
                            .arg(link.lastError().text()));
        }

        Item unfiled;
        unfiled.kind = QStringLiteral("event");
        unfiled.title = QStringLiteral("Standup");
        const std::optional<Item> createdUnfiled = items.create(unfiled);
        if (!createdUnfiled) {
            return fail(QStringLiteral(
                            "the Item without a Folder could not be created: %1")
                            .arg(items.lastError()));
        }
        unfiledId = createdUnfiled->id;
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const QList<Item> byFolder = items.readByFolder(folderId);
        if (!check(byFolder.size() == 2 && containsId(byFolder, filedId)
                       && containsId(byFolder, taggedId),
                   QStringLiteral(
                       "reading by Folder did not return the filed Items"))) {
            return 1;
        }
        if (!check(byFolder.first().folderId
                       == std::optional<qlonglong>(folderId),
                   QStringLiteral("folder_id did not round-trip unchanged"))) {
            return 1;
        }

        const QList<Item> byTag = items.readByTag(tagId);
        if (!check(byTag.size() == 1 && byTag.first().id == taggedId,
                   QStringLiteral(
                       "reading by Tag did not return the tagged Item"))) {
            return 1;
        }

        const std::optional<Item> unfiled = items.readById(unfiledId);
        if (!check(unfiled && unfiled->folderId == std::nullopt,
                   QStringLiteral(
                       "an Item without a Folder gained one across the restart"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 03, slice 4: an updated Item reads back with every changed column
// intact after a restart; created_at is preserved and the Day note's date
// query follows the update.
int updatesSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const std::optional<QString> originalDay = QStringLiteral("2026-11-24");
    const std::optional<QString> updatedDay = QStringLiteral("2026-12-05");
    const QString updatedTitle = QStringLiteral("Wednesday, revised");
    const QString updatedModifiedAt = QStringLiteral("2026-11-23T07:30:00Z");
    const std::optional<QString> updatedCompletedAt =
        QStringLiteral("2026-11-23T08:00:00Z");
    const std::optional<QString> updatedDueAt =
        QStringLiteral("2026-12-05T09:00:00Z");
    const std::optional<QString> updatedDuePrecision = QStringLiteral("day");
    const std::optional<QString> updatedStartAt =
        QStringLiteral("2026-12-05T09:00:00Z");
    const std::optional<QString> updatedEndAt =
        QStringLiteral("2026-12-05T10:00:00Z");
    const std::optional<int> updatedDurationMin = 60;
    const int updatedPriority = 2;
    const int updatedSortOrder = 9;

    qlonglong id = 0;
    QString createdAt;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item note;
        note.kind = QStringLiteral("note");
        note.title = QStringLiteral("Tuesday");
        note.day = originalDay;
        const std::optional<Item> created = items.create(note);
        if (!created) {
            return fail(QStringLiteral("the Item could not be created: %1")
                            .arg(items.lastError()));
        }
        id = created->id;
        createdAt = created->createdAt;

        const std::optional<Item> current = items.readById(id);
        if (!current) {
            return fail(QStringLiteral("the Item could not be read before update"));
        }
        Item revised = *current;
        revised.title = updatedTitle;
        revised.modifiedAt = updatedModifiedAt;
        revised.completedAt = updatedCompletedAt;
        revised.dueAt = updatedDueAt;
        revised.duePrecision = updatedDuePrecision;
        revised.startAt = updatedStartAt;
        revised.endAt = updatedEndAt;
        revised.allDay = true;
        revised.priority = updatedPriority;
        revised.durationMin = updatedDurationMin;
        revised.day = updatedDay;
        revised.sortOrder = updatedSortOrder;
        if (!items.update(revised)) {
            return fail(QStringLiteral("the Item could not be updated: %1")
                            .arg(items.lastError()));
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const std::optional<Item> read = items.readById(id);
        if (!read) {
            return fail(QStringLiteral(
                            "the updated Item was not readable after the restart: %1")
                            .arg(items.lastError()));
        }
        if (!check(read->title == updatedTitle,
                   QStringLiteral("the updated title did not survive the restart"))) {
            return 1;
        }
        if (!check(read->createdAt == createdAt,
                   QStringLiteral("created_at changed on update"))) {
            return 1;
        }
        if (!check(read->modifiedAt == updatedModifiedAt,
                   QStringLiteral("modified_at did not survive the update"))) {
            return 1;
        }
        if (!check(read->completedAt == updatedCompletedAt,
                   QStringLiteral("completed_at did not survive the update"))) {
            return 1;
        }
        if (!check(read->dueAt == updatedDueAt
                       && read->duePrecision == updatedDuePrecision
                       && read->startAt == updatedStartAt
                       && read->endAt == updatedEndAt,
                   QStringLiteral(
                       "scheduling columns did not survive the update"))) {
            return 1;
        }
        if (!check(read->allDay == true
                       && read->priority == updatedPriority
                       && read->durationMin == updatedDurationMin
                       && read->sortOrder == updatedSortOrder,
                   QStringLiteral(
                       "priority, duration, all_day or sort_order changed"))) {
            return 1;
        }
        if (!check(read->day == updatedDay
                       && read->deletedAt == std::nullopt,
                   QStringLiteral(
                       "the Day note's date or soft-delete marker changed"))) {
            return 1;
        }

        const QList<Item> oldDay = items.readByDay(*originalDay);
        if (!check(oldDay.isEmpty(),
                   QStringLiteral(
                       "the old Day note date still returns the Item"))) {
            return 1;
        }
        const QList<Item> newDay = items.readByDay(*updatedDay);
        if (!check(newDay.size() == 1 && newDay.first().id == id,
                   QStringLiteral(
                       "reading by the updated day did not return the Item"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 03, slice 5: soft-delete survives a restart. A soft-deleted Item is
// excluded from normal reads, remains in the file with deleted_at set, and is
// not cascaded to its subtasks (that rule is behavioural, a later phase).
int softDeleteSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString parentTitle = QStringLiteral("Plan the release");
    const QString subtaskTitle = QStringLiteral("Draft notes");
    qlonglong parentId = 0;
    qlonglong subtaskId = 0;

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item parent;
        parent.kind = QStringLiteral("task");
        parent.title = parentTitle;
        parent.sortOrder = 1;
        const std::optional<Item> createdParent = items.create(parent);
        if (!createdParent) {
            return fail(QStringLiteral("the parent Task could not be created: %1")
                            .arg(items.lastError()));
        }
        parentId = createdParent->id;

        Item subtask;
        subtask.kind = QStringLiteral("task");
        subtask.title = subtaskTitle;
        subtask.parentId = parentId;
        subtask.sortOrder = 2;
        const std::optional<Item> createdSubtask = items.create(subtask);
        if (!createdSubtask) {
            return fail(QStringLiteral("the subtask could not be created: %1")
                            .arg(items.lastError()));
        }
        subtaskId = createdSubtask->id;

        if (!items.softDelete(parentId)) {
            return fail(QStringLiteral("the Item could not be soft-deleted: %1")
                            .arg(items.lastError()));
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        if (!check(!items.readById(parentId),
                   QStringLiteral(
                       "a soft-deleted Item was returned by a normal read"))) {
            return 1;
        }
        const QList<Item> tasks = items.readByKind(QStringLiteral("task"));
        if (!check(tasks.size() == 1 && tasks.first().id == subtaskId,
                   QStringLiteral(
                       "a soft-deleted Item appeared in a normal query"))) {
            return 1;
        }
        const QList<Item> subtasks = items.readByParent(parentId);
        if (!check(subtasks.size() == 1 && subtasks.first().id == subtaskId
                       && subtasks.first().title == subtaskTitle,
                   QStringLiteral("soft-delete cascaded to the subtask"))) {
            return 1;
        }

        // A store-layer read observes the row that normal reads hide: proof it
        // remained in the SQLite file across the restart. This is not a Trash
        // UI and no product surface is involved.
        const std::optional<Item> stored =
            items.readByIdIncludingSoftDeleted(parentId);
        if (!check(stored.has_value(),
                   QStringLiteral(
                       "the soft-deleted row did not remain in the file"))) {
            return 1;
        }
        if (!check(stored->deletedAt.has_value()
                       && QDateTime::fromString(*stored->deletedAt,
                                                Qt::ISODate).isValid(),
                   QStringLiteral(
                       "deleted_at is not an ISO 8601 soft-delete marker"))) {
            return 1;
        }
        if (!check(stored->title == parentTitle
                       && stored->kind == QStringLiteral("task")
                       && stored->sortOrder == 1
                       && stored->parentId == std::nullopt,
                   QStringLiteral(
                       "the soft-deleted Item's other fields changed"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 03, slice 6: the one-Day-note-per-date rule is enforced with a clear
// error rather than silently, on both create and update. Foreign-key and CHECK
// constraints also reject invalid Items with a clear diagnostic, and rejected
// writes leave no stray rows behind after a restart.
int constraintViolationsAreRejectedClearly()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const std::optional<QString> day = QStringLiteral("2026-11-24");
    const std::optional<QString> otherDay = QStringLiteral("2026-11-25");
    qlonglong firstId = 0;
    qlonglong secondId = 0;

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item first;
        first.kind = QStringLiteral("note");
        first.title = QStringLiteral("Tuesday");
        first.day = day;
        const std::optional<Item> createdFirst = items.create(first);
        if (!createdFirst) {
            return fail(QStringLiteral("the first Day note could not be created: %1")
                            .arg(items.lastError()));
        }
        firstId = createdFirst->id;

        Item duplicate;
        duplicate.kind = QStringLiteral("note");
        duplicate.title = QStringLiteral("Another Tuesday");
        duplicate.day = day;
        if (!check(!items.create(duplicate),
                   QStringLiteral(
                       "a second Day note for the same date was accepted"))) {
            return 1;
        }
        if (!check(items.lastError().contains(QStringLiteral("Day note")),
                   QStringLiteral(
                       "a duplicate Day note did not produce a clear error"))) {
            return 1;
        }

        Item second;
        second.kind = QStringLiteral("note");
        second.title = QStringLiteral("Wednesday");
        second.day = otherDay;
        const std::optional<Item> createdSecond = items.create(second);
        if (!createdSecond) {
            return fail(QStringLiteral("the second Day note could not be created: %1")
                            .arg(items.lastError()));
        }
        secondId = createdSecond->id;

        const std::optional<Item> current = items.readById(secondId);
        if (!current) {
            return fail(QStringLiteral(
                            "the second Day note could not be read before update"));
        }
        Item moved = *current;
        moved.day = day;
        if (!check(!items.update(moved),
                   QStringLiteral(
                       "updating a Day note to a duplicate date was accepted"))) {
            return 1;
        }
        if (!check(items.lastError().contains(QStringLiteral("Day note")),
                   QStringLiteral(
                       "a duplicate day on update did not produce a clear error"))) {
            return 1;
        }

        // Foreign-key enforcement stays on: a subtask needs a real parent Item.
        Item orphan;
        orphan.kind = QStringLiteral("task");
        orphan.title = QStringLiteral("Orphan");
        orphan.parentId = 999999;
        if (!check(!items.create(orphan),
                   QStringLiteral(
                       "an Item with an unknown parent was accepted"))) {
            return 1;
        }
        if (!check(items.lastError().contains(QStringLiteral("FOREIGN KEY")),
                   QStringLiteral(
                       "a foreign-key violation did not produce a clear error"))) {
            return 1;
        }

        // The items.kind CHECK constraint rejects an unknown kind.
        Item invalid;
        invalid.kind = QStringLiteral("invalid-kind");
        invalid.title = QStringLiteral("Invalid");
        if (!check(!items.create(invalid),
                   QStringLiteral("an Item with an invalid kind was accepted"))) {
            return 1;
        }
        if (!check(items.lastError().contains(QStringLiteral("CHECK")),
                   QStringLiteral(
                       "a CHECK violation did not produce a clear error"))) {
            return 1;
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const QList<Item> byDay = items.readByDay(*day);
        if (!check(byDay.size() == 1 && byDay.first().id == firstId,
                   QStringLiteral(
                       "the rejected duplicate Day note left a stray row"))) {
            return 1;
        }
        const QList<Item> byOtherDay = items.readByDay(*otherDay);
        if (!check(byOtherDay.size() == 1 && byOtherDay.first().id == secondId,
                   QStringLiteral(
                       "the rejected day update moved the Day note"))) {
            return 1;
        }
        const QList<Item> notes = items.readByKind(QStringLiteral("note"));
        if (!check(notes.size() == 2,
                   QStringLiteral("rejected writes left stray Items behind"))) {
            return 1;
        }
    }

    return 0;
}

// Regression: a soft-deleted Day note no longer reserves its date. The
// one_day_note_per_day unique index covers only active rows (deleted_at IS NULL),
// matching normal reads, so deleting a Day note and creating a replacement for
// the same date is allowed and survives a restart. The rule still rejects a
// second *active* Day note for the same date, with a clear error.
int deletedDayNoteDateCanBeReused()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const std::optional<QString> day = QStringLiteral("2026-11-24");
    const QString replacementTitle = QStringLiteral("Tuesday, replacement");
    qlonglong firstId = 0;
    qlonglong replacementId = 0;

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item first;
        first.kind = QStringLiteral("note");
        first.title = QStringLiteral("Tuesday");
        first.day = day;
        const std::optional<Item> createdFirst = items.create(first);
        if (!createdFirst) {
            return fail(QStringLiteral(
                            "the first Day note could not be created: %1")
                            .arg(items.lastError()));
        }
        firstId = createdFirst->id;

        // A second active Day note for the same date is still rejected.
        Item activeDuplicate;
        activeDuplicate.kind = QStringLiteral("note");
        activeDuplicate.title = QStringLiteral("Another Tuesday");
        activeDuplicate.day = day;
        if (!check(!items.create(activeDuplicate),
                   QStringLiteral(
                       "a second active Day note for the same date was accepted"))) {
            return 1;
        }
        if (!check(items.lastError().contains(QStringLiteral("Day note")),
                   QStringLiteral(
                       "a duplicate active Day note did not produce a clear error"))) {
            return 1;
        }

        if (!items.softDelete(firstId)) {
            return fail(QStringLiteral(
                            "the Day note could not be soft-deleted: %1")
                            .arg(items.lastError()));
        }

        // The soft-deleted Day note's date is now reusable.
        Item replacement;
        replacement.kind = QStringLiteral("note");
        replacement.title = replacementTitle;
        replacement.day = day;
        const std::optional<Item> createdReplacement = items.create(replacement);
        if (!createdReplacement) {
            return fail(QStringLiteral(
                            "a replacement Day note for the soft-deleted date "
                            "could not be created: %1")
                            .arg(items.lastError()));
        }
        replacementId = createdReplacement->id;
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const QList<Item> byDay = items.readByDay(*day);
        if (!check(byDay.size() == 1
                       && byDay.first().id == replacementId
                       && byDay.first().title == replacementTitle,
                   QStringLiteral(
                       "reading by day did not return only the replacement "
                       "Day note after the restart"))) {
            return 1;
        }

        const std::optional<Item> replacement = items.readById(replacementId);
        if (!check(replacement && replacement->day == day
                       && replacement->deletedAt == std::nullopt,
                   QStringLiteral(
                       "the replacement Day note did not survive the restart"))) {
            return 1;
        }

        // The soft-deleted row stays in the file, hidden from normal reads, and
        // keeps its date: the fix is the index predicate, not data loss.
        if (!check(!items.readById(firstId),
                   QStringLiteral(
                       "the soft-deleted Day note was returned by a normal read"))) {
            return 1;
        }
        const std::optional<Item> stored =
            items.readByIdIncludingSoftDeleted(firstId);
        if (!check(stored && stored->deletedAt.has_value()
                       && stored->day == day,
                   QStringLiteral(
                       "the soft-deleted Day note lost its date or marker"))) {
            return 1;
        }

        // The one-active-note-per-date rule still holds after the restart.
        Item third;
        third.kind = QStringLiteral("note");
        third.title = QStringLiteral("A third Tuesday");
        third.day = day;
        if (!check(!items.create(third),
                   QStringLiteral(
                       "a second active Day note for the date was accepted "
                       "after the restart"))) {
            return 1;
        }
        if (!check(items.lastError().contains(QStringLiteral("Day note")),
                   QStringLiteral(
                       "a duplicate active Day note did not produce a clear "
                       "error after the restart"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 04, slice 1: one Block written before a restart reads back with every
// field unchanged — position, type, content, inline_json, task_id and indent.
// The expected values are independent literals, never recomputed by the store.
// task_id points at a real Task Item, so foreign-key enforcement is exercised
// through the public repository API as well.
int aBlockSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString noteTitle = QStringLiteral("Tuesday");
    const QString taskTitle = QStringLiteral("Reply to Anna");
    const QString blockType = QStringLiteral("text");
    const QString content = QStringLiteral("First line of the Note");
    const std::optional<QString> inlineJson = QStringLiteral(
        "[{\"start\":0,\"end\":5,\"type\":\"bold\"},"
        "{\"start\":6,\"end\":21,\"type\":\"link\","
        "\"href\":\"https://example.invalid\"}]");
    const int indent = 2;

    qlonglong noteId = 0;
    qlonglong taskId = 0;
    Block written;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item note;
        note.kind = QStringLiteral("note");
        note.title = noteTitle;
        const std::optional<Item> createdNote = items.create(note);
        if (!createdNote) {
            return fail(QStringLiteral("the Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteId = createdNote->id;

        Item task;
        task.kind = QStringLiteral("task");
        task.title = taskTitle;
        const std::optional<Item> createdTask = items.create(task);
        if (!createdTask) {
            return fail(QStringLiteral("the Task could not be created: %1")
                            .arg(items.lastError()));
        }
        taskId = createdTask->id;

        BlockRepository blocks(database);
        Block block;
        block.itemId = noteId;
        block.type = blockType;
        block.content = content;
        block.inlineJson = inlineJson;
        block.taskId = taskId;
        block.indent = indent;
        const std::optional<Block> created = blocks.create(block);
        if (!created) {
            return fail(QStringLiteral("the Block could not be created: %1")
                            .arg(blocks.lastError()));
        }
        written = *created;
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);
        const QList<Block> read = blocks.readByItem(noteId);
        if (!check(read.size() == 1,
                   QStringLiteral(
                       "reading the Note's Blocks did not return one Block"))) {
            return 1;
        }
        const Block& block = read.first();
        if (!check(block.id == written.id,
                   QStringLiteral("the Block id changed across the restart"))) {
            return 1;
        }
        if (!check(block.itemId == noteId,
                   QStringLiteral("the owning Item id did not round-trip"))) {
            return 1;
        }
        if (!check(block.position == 0,
                   QStringLiteral("the first Block is not at position 0"))) {
            return 1;
        }
        if (!check(block.type == blockType,
                   QStringLiteral("type did not round-trip unchanged"))) {
            return 1;
        }
        if (!check(block.content == content,
                   QStringLiteral("content did not round-trip unchanged"))) {
            return 1;
        }
        if (!check(block.inlineJson == inlineJson,
                   QStringLiteral("inline_json did not round-trip unchanged"))) {
            return 1;
        }
        if (!check(block.taskId == std::optional<qlonglong>(taskId),
                   QStringLiteral("task_id did not round-trip unchanged"))) {
            return 1;
        }
        if (!check(block.indent == indent,
                   QStringLiteral("indent did not round-trip unchanged"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 04, slice 2: Blocks of all five types read back in the same order
// within their owning Item after a restart, with inline_json, task_id and
// indent intact. A second Note's Blocks stay separate; a Block is readable by
// id; and CHECK / foreign-key violations are rejected with clear errors,
// leaving no stray rows. A to-do Block is a Block type, not a synonym for a
// Task: the Task it points at is a separate Item.
int blocksOfAllTypesSurviveRestart()
{
    struct BlockSpec {
        QString type;
        QString content;
        std::optional<QString> inlineJson;
        std::optional<qlonglong> taskId;
        int indent = 0;
    };

    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const std::optional<QString> inlineJson = QStringLiteral(
        "[{\"start\":0,\"end\":3,\"type\":\"italic\"}]");
    const QString otherContent = QStringLiteral("A line in another Note");
    QList<BlockSpec> specs = {
        { QStringLiteral("text"),    QStringLiteral("Plain line"),
          std::nullopt, std::nullopt, 0 },
        { QStringLiteral("heading"), QStringLiteral("A heading"),
          std::nullopt, std::nullopt, 0 },
        { QStringLiteral("todo"),    QStringLiteral("A to-do line"),
          inlineJson, std::nullopt, 1 },
        { QStringLiteral("bullet"),  QStringLiteral("A bullet"),
          std::nullopt, std::nullopt, 2 },
        { QStringLiteral("code"),    QStringLiteral("int main() { return 0; }"),
          std::nullopt, std::nullopt, 0 },
    };

    qlonglong noteAId = 0;
    qlonglong noteBId = 0;
    qlonglong otherId = 0;
    QList<qlonglong> ids;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item noteA;
        noteA.kind = QStringLiteral("note");
        noteA.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdA = items.create(noteA);
        if (!createdA) {
            return fail(QStringLiteral("the first Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteAId = createdA->id;

        Item noteB;
        noteB.kind = QStringLiteral("note");
        noteB.title = QStringLiteral("Wednesday");
        const std::optional<Item> createdB = items.create(noteB);
        if (!createdB) {
            return fail(QStringLiteral("the second Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteBId = createdB->id;

        Item task;
        task.kind = QStringLiteral("task");
        task.title = QStringLiteral("Reply to Anna");
        const std::optional<Item> createdTask = items.create(task);
        if (!createdTask) {
            return fail(QStringLiteral("the Task could not be created: %1")
                            .arg(items.lastError()));
        }
        specs[2].taskId = createdTask->id;

        BlockRepository blocks(database);
        for (const BlockSpec& spec : specs) {
            Block block;
            block.itemId = noteAId;
            block.type = spec.type;
            block.content = spec.content;
            block.inlineJson = spec.inlineJson;
            block.taskId = spec.taskId;
            block.indent = spec.indent;
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral(
                                "a Block of type '%1' could not be created: %2")
                                .arg(spec.type, blocks.lastError()));
            }
            ids.append(created->id);
        }

        Block other;
        other.itemId = noteBId;
        other.type = QStringLiteral("text");
        other.content = otherContent;
        const std::optional<Block> createdOther = blocks.create(other);
        if (!createdOther) {
            return fail(QStringLiteral(
                            "the other Note's Block could not be created: %1")
                            .arg(blocks.lastError()));
        }
        otherId = createdOther->id;

        // The five-type CHECK constraint rejects an unknown Block type.
        Block invalidType;
        invalidType.itemId = noteAId;
        invalidType.type = QStringLiteral("quote");
        if (!check(!blocks.create(invalidType),
                   QStringLiteral("an invalid Block type was accepted"))) {
            return 1;
        }
        if (!check(blocks.lastError().contains(QStringLiteral("CHECK")),
                   QStringLiteral(
                       "an invalid Block type did not produce a clear error: %1")
                       .arg(blocks.lastError()))) {
            return 1;
        }

        // Foreign-key enforcement stays on: a Block needs a real owning Item.
        Block unknownItem;
        unknownItem.itemId = 999999;
        unknownItem.type = QStringLiteral("text");
        if (!check(!blocks.create(unknownItem),
                   QStringLiteral(
                       "a Block with an unknown owning Item was accepted"))) {
            return 1;
        }
        if (!check(blocks.lastError().contains(QStringLiteral("FOREIGN KEY")),
                   QStringLiteral(
                       "a foreign-key violation did not produce a clear error"))) {
            return 1;
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        // Exactly the five written Blocks remain: rejected writes left no rows.
        const QList<Block> read = blocks.readByItem(noteAId);
        if (!check(read.size() == specs.size(),
                   QStringLiteral(
                       "the Note's Blocks did not all survive the restart"))) {
            return 1;
        }
        for (int i = 0; i < specs.size(); ++i) {
            const Block& block = read.at(i);
            const BlockSpec& spec = specs.at(i);
            if (!check(block.id == ids.at(i) && block.itemId == noteAId
                           && block.position == i,
                       QStringLiteral(
                           "a Block's id, owning Item or position changed"))) {
                return 1;
            }
            if (!check(block.type == spec.type && block.content == spec.content,
                       QStringLiteral(
                           "a Block's type or content did not round-trip"))) {
                return 1;
            }
            if (!check(block.inlineJson == spec.inlineJson
                           && block.taskId == spec.taskId
                           && block.indent == spec.indent,
                       QStringLiteral(
                           "inline_json, task_id or indent changed"))) {
                return 1;
            }
        }

        const std::optional<Block> byId = blocks.readById(ids.at(2));
        if (!check(byId && byId->id == ids.at(2)
                       && byId->type == QStringLiteral("todo")
                       && byId->taskId == specs.at(2).taskId,
                   QStringLiteral("a Block could not be read back by id"))) {
            return 1;
        }
        if (!check(!blocks.readById(999999),
                   QStringLiteral("an absent Block id returned a Block"))) {
            return 1;
        }

        const QList<Block> other = blocks.readByItem(noteBId);
        if (!check(other.size() == 1 && other.first().id == otherId
                       && other.first().content == otherContent
                       && other.first().position == 0,
                   QStringLiteral(
                       "another Note's Blocks were mixed into this Note"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 04, slice 3: an updated Block reads back with every changed column
// intact after a restart, while the surrounding Blocks keep their order and
// their own values. Updating an absent Block reports a clear error.
int blockUpdatesSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString firstContent = QStringLiteral("First line");
    const QString secondContent = QStringLiteral("Second line");
    const QString thirdContent = QStringLiteral("Third line");
    const QString updatedContent = QStringLiteral("Revised second line");
    const std::optional<QString> updatedInlineJson = QStringLiteral(
        "[{\"start\":0,\"end\":7,\"type\":\"bold\"}]");
    const int updatedIndent = 3;

    qlonglong noteId = 0;
    qlonglong taskId = 0;
    QList<qlonglong> ids;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item note;
        note.kind = QStringLiteral("note");
        note.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdNote = items.create(note);
        if (!createdNote) {
            return fail(QStringLiteral("the Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteId = createdNote->id;

        Item task;
        task.kind = QStringLiteral("task");
        task.title = QStringLiteral("Reply to Anna");
        const std::optional<Item> createdTask = items.create(task);
        if (!createdTask) {
            return fail(QStringLiteral("the Task could not be created: %1")
                            .arg(items.lastError()));
        }
        taskId = createdTask->id;

        BlockRepository blocks(database);
        const QStringList types = { QStringLiteral("text"),
                                    QStringLiteral("heading"),
                                    QStringLiteral("bullet") };
        const QStringList contents = { firstContent, secondContent,
                                       thirdContent };
        for (int i = 0; i < types.size(); ++i) {
            Block block;
            block.itemId = noteId;
            block.type = types.at(i);
            block.content = contents.at(i);
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral("a Block could not be created: %1")
                                .arg(blocks.lastError()));
            }
            ids.append(created->id);
        }

        const QList<Block> current = blocks.readByItem(noteId);
        if (!check(current.size() == 3,
                   QStringLiteral(
                       "the Note's three Blocks could not be read before update"))) {
            return 1;
        }
        Block revised = current.at(1);
        revised.type = QStringLiteral("code");
        revised.content = updatedContent;
        revised.inlineJson = updatedInlineJson;
        revised.taskId = taskId;
        revised.indent = updatedIndent;
        if (!blocks.update(revised)) {
            return fail(QStringLiteral("the Block could not be updated: %1")
                            .arg(blocks.lastError()));
        }

        Block absent = revised;
        absent.id = 999999;
        if (!check(!blocks.update(absent),
                   QStringLiteral("updating an absent Block succeeded"))) {
            return 1;
        }
        if (!check(blocks.lastError().contains(QStringLiteral("no Block")),
                   QStringLiteral(
                       "updating an absent Block did not report a clear error: %1")
                       .arg(blocks.lastError()))) {
            return 1;
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        const QList<Block> read = blocks.readByItem(noteId);
        if (!check(read.size() == 3,
                   QStringLiteral(
                       "the Note's Blocks did not all survive the restart"))) {
            return 1;
        }
        for (int i = 0; i < read.size(); ++i) {
            if (!check(read.at(i).id == ids.at(i)
                           && read.at(i).position == i
                           && read.at(i).itemId == noteId,
                       QStringLiteral(
                           "the Block order or owning Item changed on update"))) {
                return 1;
            }
        }
        if (!check(read.at(0).type == QStringLiteral("text")
                       && read.at(0).content == firstContent
                       && read.at(0).inlineJson == std::nullopt
                       && read.at(0).taskId == std::nullopt
                       && read.at(0).indent == 0,
                   QStringLiteral("the first Block changed on update"))) {
            return 1;
        }
        if (!check(read.at(2).type == QStringLiteral("bullet")
                       && read.at(2).content == thirdContent
                       && read.at(2).indent == 0,
                   QStringLiteral("the third Block changed on update"))) {
            return 1;
        }
        const Block& changed = read.at(1);
        if (!check(changed.type == QStringLiteral("code")
                       && changed.content == updatedContent
                       && changed.inlineJson == updatedInlineJson
                       && changed.taskId == std::optional<qlonglong>(taskId)
                       && changed.indent == updatedIndent,
                   QStringLiteral(
                       "the updated Block's columns did not survive the restart"))) {
            return 1;
        }

        const std::optional<Block> byId = blocks.readById(ids.at(1));
        if (!check(byId && byId->type == changed.type
                       && byId->content == changed.content,
                   QStringLiteral(
                       "reading the updated Block by id disagreed"))) {
            return 1;
        }
    }

    return 0;
}

// Code-review finding on update(): an ordinary update writes only a Block's
// content columns, so a stale read or a cross-Item value carried in the Block
// can never re-own the row or break the contiguous per-Item ordering. This
// attempts both an owner change and a position change through update(), checks
// that the content still updates while owner, position and order stay intact,
// shows that reorder() is the route for a real position change, and verifies
// the result across a restart.
int blockUpdatePreservesOwnerAndPosition()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QStringList contents = { QStringLiteral("Line A"),
                                   QStringLiteral("Line B"),
                                   QStringLiteral("Line C") };
    const QString ownerAttemptContent = QStringLiteral("Owner attempt");
    const QString positionAttemptContent = QStringLiteral("Position attempt");
    const QString contentOnlyText = QStringLiteral("Content-only edit");

    qlonglong noteAId = 0;
    qlonglong noteBId = 0;
    QList<qlonglong> ids;
    QList<qlonglong> expectedOrder; // the order after reorder(), for the restart
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item noteA;
        noteA.kind = QStringLiteral("note");
        noteA.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdA = items.create(noteA);
        if (!createdA) {
            return fail(QStringLiteral("the first Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteAId = createdA->id;

        Item noteB;
        noteB.kind = QStringLiteral("note");
        noteB.title = QStringLiteral("Wednesday");
        const std::optional<Item> createdB = items.create(noteB);
        if (!createdB) {
            return fail(QStringLiteral("the second Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteBId = createdB->id;

        BlockRepository blocks(database);
        for (const QString& content : contents) {
            Block block;
            block.itemId = noteAId;
            block.type = QStringLiteral("text");
            block.content = content;
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral("a Block could not be created: %1")
                                .arg(blocks.lastError()));
            }
            ids.append(created->id);
        }

        // One real Block in the other Note, so the cross-Item owner value below
        // points at an existing Item rather than an invalid foreign key.
        Block other;
        other.itemId = noteBId;
        other.type = QStringLiteral("text");
        other.content = QStringLiteral("A line in the other Note");
        if (!blocks.create(other)) {
            return fail(QStringLiteral(
                            "the other Note's Block could not be created: %1")
                            .arg(blocks.lastError()));
        }

        const QList<Block> current = blocks.readByItem(noteAId);
        if (!check(current.size() == 3,
                   QStringLiteral("the Note's Blocks could not be read"))) {
            return 1;
        }

        // (1) Attempt to re-own a Block: the caller's itemId names the other
        // Note. update() must keep the stored owner and position.
        Block ownerAttempt = current.at(1);
        ownerAttempt.itemId = noteBId;
        ownerAttempt.content = ownerAttemptContent;
        if (!blocks.update(ownerAttempt)) {
            return fail(QStringLiteral(
                            "the owner-change update could not be applied: %1")
                            .arg(blocks.lastError()));
        }

        // (2) Attempt to reposition a Block: a stale, arbitrary position that
        // would collide with the first Block. update() must keep position 2.
        Block positionAttempt = current.at(2);
        positionAttempt.position = 0;
        positionAttempt.content = positionAttemptContent;
        if (!blocks.update(positionAttempt)) {
            return fail(QStringLiteral(
                            "the position-change update could not be applied: %1")
                            .arg(blocks.lastError()));
        }

        // (3) A content-only Block (owner/position left at their defaults) also
        // updates without disturbing the stored owner or position.
        Block contentOnly;
        contentOnly.id = ids.at(0);
        contentOnly.type = QStringLiteral("text");
        contentOnly.content = contentOnlyText;
        if (!blocks.update(contentOnly)) {
            return fail(QStringLiteral(
                            "the content-only update could not be applied: %1")
                            .arg(blocks.lastError()));
        }

        // Before the restart: content edits landed, but owner, positions and
        // contiguous ordering are unchanged; the other Note is untouched.
        const QList<Block> after = blocks.readByItem(noteAId);
        if (!check(after.size() == 3,
                   QStringLiteral("update changed the number of Blocks"))) {
            return 1;
        }
        for (int i = 0; i < after.size(); ++i) {
            if (!check(after.at(i).id == ids.at(i)
                           && after.at(i).position == i
                           && after.at(i).itemId == noteAId,
                       QStringLiteral(
                           "update changed a Block's owner or position"))) {
                return 1;
            }
        }
        if (!check(after.at(0).content == contentOnlyText
                       && after.at(1).content == ownerAttemptContent
                       && after.at(2).content == positionAttemptContent,
                   QStringLiteral("update did not write the content edits"))) {
            return 1;
        }
        const QList<Block> otherAfter = blocks.readByItem(noteBId);
        if (!check(otherAfter.size() == 1
                       && otherAfter.first().itemId == noteBId
                       && otherAfter.first().position == 0,
                   QStringLiteral(
                       "a cross-Item update disturbed the other Note"))) {
            return 1;
        }

        // A real position change goes through reorder(), which keeps positions
        // contiguous from 0 instead of being smuggled through update().
        expectedOrder = { ids.at(2), ids.at(0), ids.at(1) };
        if (!blocks.reorder(noteAId, expectedOrder)) {
            return fail(QStringLiteral("the Blocks could not be reordered: %1")
                            .arg(blocks.lastError()));
        }
        const QList<Block> reordered = blocks.readByItem(noteAId);
        if (!check(reordered.size() == 3
                       && reordered.at(0).id == expectedOrder.at(0)
                       && reordered.at(0).position == 0
                       && reordered.at(1).id == expectedOrder.at(1)
                       && reordered.at(1).position == 1
                       && reordered.at(2).id == expectedOrder.at(2)
                       && reordered.at(2).position == 2,
                   QStringLiteral("reorder() did not re-sequence positions"))) {
            return 1;
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        const QList<Block> read = blocks.readByItem(noteAId);
        if (!check(read.size() == expectedOrder.size(),
                   QStringLiteral(
                       "the reordered Blocks did not survive the restart"))) {
            return 1;
        }
        for (int i = 0; i < expectedOrder.size(); ++i) {
            if (!check(read.at(i).id == expectedOrder.at(i)
                           && read.at(i).position == i
                           && read.at(i).itemId == noteAId,
                       QStringLiteral(
                           "owner or position changed across the restart"))) {
                return 1;
            }
        }
        const QList<Block> other = blocks.readByItem(noteBId);
        if (!check(other.size() == 1 && other.first().itemId == noteBId
                       && other.first().position == 0,
                   QStringLiteral(
                       "the other Note's Block was disturbed across the restart"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 04, slice 4: deleting a Block re-sequences the positions of the
// Blocks that remain, and the resulting order survives a restart. Deleting an
// absent Block reports a clear error and leaves the other Blocks untouched.
int blockDeletionResequencesAndSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QStringList contents = { QStringLiteral("Line A"),
                                   QStringLiteral("Line B"),
                                   QStringLiteral("Line C"),
                                   QStringLiteral("Line D") };

    qlonglong noteId = 0;
    QList<qlonglong> ids;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item note;
        note.kind = QStringLiteral("note");
        note.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdNote = items.create(note);
        if (!createdNote) {
            return fail(QStringLiteral("the Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteId = createdNote->id;

        BlockRepository blocks(database);
        for (const QString& content : contents) {
            Block block;
            block.itemId = noteId;
            block.type = QStringLiteral("text");
            block.content = content;
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral("a Block could not be created: %1")
                                .arg(blocks.lastError()));
            }
            ids.append(created->id);
        }

        if (!blocks.remove(ids.at(1))) {
            return fail(QStringLiteral("the second Block could not be deleted: %1")
                            .arg(blocks.lastError()));
        }
        if (!blocks.remove(ids.at(0))) {
            return fail(QStringLiteral("the first Block could not be deleted: %1")
                            .arg(blocks.lastError()));
        }
        if (!check(!blocks.remove(999999),
                   QStringLiteral("deleting an absent Block succeeded"))) {
            return 1;
        }
        if (!check(blocks.lastError().contains(QStringLiteral("no Block")),
                   QStringLiteral(
                       "deleting an absent Block did not report a clear error: %1")
                       .arg(blocks.lastError()))) {
            return 1;
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        const QList<Block> read = blocks.readByItem(noteId);
        if (!check(read.size() == 2,
                   QStringLiteral(
                       "deleting two Blocks did not leave two Blocks"))) {
            return 1;
        }
        if (!check(read.at(0).id == ids.at(2) && read.at(0).position == 0
                       && read.at(0).content == contents.at(2),
                   QStringLiteral(
                       "the first remaining Block was not re-sequenced"))) {
            return 1;
        }
        if (!check(read.at(1).id == ids.at(3) && read.at(1).position == 1
                       && read.at(1).content == contents.at(3),
                   QStringLiteral(
                       "the second remaining Block was not re-sequenced"))) {
            return 1;
        }
        if (!check(!blocks.readById(ids.at(0)) && !blocks.readById(ids.at(1)),
                   QStringLiteral("a deleted Block was still readable by id"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 04, slice 5 — the ticket's headline persistence seam: a Note's Blocks
// written and reordered before a restart read back in the same order after the
// connection is reopened. A reorder that names a foreign Block is rejected
// clearly and leaves the stored order untouched.
int blockReorderSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QStringList contents = { QStringLiteral("Line A"),
                                   QStringLiteral("Line B"),
                                   QStringLiteral("Line C"),
                                   QStringLiteral("Line D"),
                                   QStringLiteral("Line E") };
    const QStringList types = { QStringLiteral("text"),
                                QStringLiteral("heading"),
                                QStringLiteral("todo"),
                                QStringLiteral("bullet"),
                                QStringLiteral("code") };

    qlonglong noteAId = 0;
    qlonglong noteBId = 0;
    qlonglong foreignId = 0;
    QList<qlonglong> ids;
    QList<qlonglong> perm;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item noteA;
        noteA.kind = QStringLiteral("note");
        noteA.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdA = items.create(noteA);
        if (!createdA) {
            return fail(QStringLiteral("the first Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteAId = createdA->id;

        Item noteB;
        noteB.kind = QStringLiteral("note");
        noteB.title = QStringLiteral("Wednesday");
        const std::optional<Item> createdB = items.create(noteB);
        if (!createdB) {
            return fail(QStringLiteral("the second Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteBId = createdB->id;

        BlockRepository blocks(database);
        for (int i = 0; i < contents.size(); ++i) {
            Block block;
            block.itemId = noteAId;
            block.type = types.at(i);
            block.content = contents.at(i);
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral("a Block could not be created: %1")
                                .arg(blocks.lastError()));
            }
            ids.append(created->id);
        }

        Block foreign;
        foreign.itemId = noteBId;
        foreign.type = QStringLiteral("text");
        foreign.content = QStringLiteral("A line in another Note");
        const std::optional<Block> createdForeign = blocks.create(foreign);
        if (!createdForeign) {
            return fail(QStringLiteral(
                            "the other Note's Block could not be created: %1")
                            .arg(blocks.lastError()));
        }
        foreignId = createdForeign->id;

        // A reorder may only name this Item's own Blocks.
        const QList<qlonglong> withForeign = { ids.at(0), ids.at(1), foreignId,
                                              ids.at(2), ids.at(3) };
        if (!check(!blocks.reorder(noteAId, withForeign),
                   QStringLiteral("a reorder naming a foreign Block succeeded"))) {
            return 1;
        }
        if (!check(blocks.lastError().contains(QStringLiteral("reorder")),
                   QStringLiteral(
                       "a rejected reorder did not report a clear error: %1")
                       .arg(blocks.lastError()))) {
            return 1;
        }
        const QList<Block> afterRejection = blocks.readByItem(noteAId);
        if (!check(afterRejection.size() == ids.size()
                       && afterRejection.at(2).id == ids.at(2)
                       && afterRejection.at(2).position == 2,
                   QStringLiteral("a rejected reorder changed the stored order"))) {
            return 1;
        }

        perm = { ids.at(3), ids.at(0), ids.at(4), ids.at(2), ids.at(1) };
        if (!blocks.reorder(noteAId, perm)) {
            return fail(QStringLiteral("the Blocks could not be reordered: %1")
                            .arg(blocks.lastError()));
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        const QList<Block> read = blocks.readByItem(noteAId);
        if (!check(read.size() == perm.size(),
                   QStringLiteral(
                       "the reordered Blocks did not all survive the restart"))) {
            return 1;
        }
        for (int i = 0; i < perm.size(); ++i) {
            if (!check(read.at(i).id == perm.at(i) && read.at(i).position == i,
                       QStringLiteral(
                           "the reordered Block order changed across the restart"))) {
                return 1;
            }
        }
        // Independent worked example: the new order is D, A, E, C, B.
        const QStringList expected = { contents.at(3), contents.at(0),
                                       contents.at(4), contents.at(2),
                                       contents.at(1) };
        for (int i = 0; i < expected.size(); ++i) {
            if (!check(read.at(i).content == expected.at(i),
                       QStringLiteral(
                           "a reordered Block's content changed"))) {
                return 1;
            }
        }

        const std::optional<Block> byId = blocks.readById(ids.at(3));
        if (!check(byId && byId->position == 0,
                   QStringLiteral(
                       "reading a reordered Block by id disagreed"))) {
            return 1;
        }
        const QList<Block> other = blocks.readByItem(noteBId);
        if (!check(other.size() == 1 && other.first().id == foreignId
                       && other.first().position == 0,
                   QStringLiteral(
                       "the other Note's Block was disturbed by the reorder"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 04, slice 6: inserting a Block at an explicit position re-sequences
// the positions of the Blocks already in the owning Item, and that order
// survives a restart. A position past the end clamps to an append.
int blockInsertionResequencesAndSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString lineA = QStringLiteral("Line A");
    const QString lineB = QStringLiteral("Line B");
    const QString lineC = QStringLiteral("Line C");
    const QString first = QStringLiteral("Inserted first");
    const QString middle = QStringLiteral("Inserted middle");
    const QString appended = QStringLiteral("Appended last");

    qlonglong noteId = 0;
    QList<qlonglong> expectedIds;
    QStringList expectedContents;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item note;
        note.kind = QStringLiteral("note");
        note.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdNote = items.create(note);
        if (!createdNote) {
            return fail(QStringLiteral("the Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteId = createdNote->id;

        BlockRepository blocks(database);
        QList<qlonglong> originalIds;
        for (const QString& content : { lineA, lineB, lineC }) {
            Block block;
            block.itemId = noteId;
            block.type = QStringLiteral("text");
            block.content = content;
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral("a Block could not be created: %1")
                                .arg(blocks.lastError()));
            }
            originalIds.append(created->id);
        }

        Block insertMiddle;
        insertMiddle.itemId = noteId;
        insertMiddle.type = QStringLiteral("text");
        insertMiddle.content = middle;
        insertMiddle.position = 1;
        const std::optional<Block> createdMiddle = blocks.create(insertMiddle);
        if (!createdMiddle) {
            return fail(QStringLiteral(
                            "the middle Block could not be inserted: %1")
                            .arg(blocks.lastError()));
        }
        if (!check(createdMiddle->position == 1,
                   QStringLiteral(
                       "the middle Block was not inserted at position 1"))) {
            return 1;
        }

        Block insertFirst;
        insertFirst.itemId = noteId;
        insertFirst.type = QStringLiteral("text");
        insertFirst.content = first;
        insertFirst.position = 0;
        const std::optional<Block> createdFirst = blocks.create(insertFirst);
        if (!createdFirst) {
            return fail(QStringLiteral(
                            "the first Block could not be inserted: %1")
                            .arg(blocks.lastError()));
        }

        Block insertPastEnd;
        insertPastEnd.itemId = noteId;
        insertPastEnd.type = QStringLiteral("text");
        insertPastEnd.content = appended;
        insertPastEnd.position = 99;
        const std::optional<Block> createdAppended =
            blocks.create(insertPastEnd);
        if (!createdAppended) {
            return fail(QStringLiteral(
                            "the trailing Block could not be inserted: %1")
                            .arg(blocks.lastError()));
        }
        if (!check(createdAppended->position == 5,
                   QStringLiteral(
                       "a position past the end did not clamp to an append"))) {
            return 1;
        }

        expectedIds = { createdFirst->id, originalIds.at(0),
                        createdMiddle->id, originalIds.at(1),
                        originalIds.at(2), createdAppended->id };
        expectedContents = { first, lineA, middle, lineB, lineC, appended };
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        const QList<Block> read = blocks.readByItem(noteId);
        if (!check(read.size() == expectedIds.size(),
                   QStringLiteral(
                       "inserting Blocks did not leave the expected count"))) {
            return 1;
        }
        for (int i = 0; i < expectedIds.size(); ++i) {
            if (!check(read.at(i).id == expectedIds.at(i)
                           && read.at(i).position == i
                           && read.at(i).content == expectedContents.at(i),
                       QStringLiteral(
                           "the re-sequenced Block order changed across the "
                           "restart"))) {
                return 1;
            }
        }
    }

    return 0;
}

// Issue 04, slice 7: the schema's ON DELETE CASCADE removes an owning Item's
// Blocks and leaves no orphan rows, observed through BlockRepository across a
// restart. ItemRepository exposes only soft-delete (ticket 03), so the cascade
// contract of the schema is triggered with a bound-value raw DELETE as fixture
// setup; every assertion still reads through the BlockRepository interface.
int deletingAnItemCascadesToItsBlocks()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString otherContent = QStringLiteral("A line in another Note");

    qlonglong noteAId = 0;
    qlonglong noteBId = 0;
    QList<qlonglong> cascadeIds;
    qlonglong otherId = 0;
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item noteA;
        noteA.kind = QStringLiteral("note");
        noteA.title = QStringLiteral("Tuesday");
        const std::optional<Item> createdA = items.create(noteA);
        if (!createdA) {
            return fail(QStringLiteral("the first Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteAId = createdA->id;

        Item noteB;
        noteB.kind = QStringLiteral("note");
        noteB.title = QStringLiteral("Wednesday");
        const std::optional<Item> createdB = items.create(noteB);
        if (!createdB) {
            return fail(QStringLiteral("the second Note could not be created: %1")
                            .arg(items.lastError()));
        }
        noteBId = createdB->id;

        Item task;
        task.kind = QStringLiteral("task");
        task.title = QStringLiteral("Reply to Anna");
        const std::optional<Item> createdTask = items.create(task);
        if (!createdTask) {
            return fail(QStringLiteral("the Task could not be created: %1")
                            .arg(items.lastError()));
        }

        BlockRepository blocks(database);
        const QStringList types = { QStringLiteral("text"),
                                    QStringLiteral("todo"),
                                    QStringLiteral("code") };
        for (int i = 0; i < types.size(); ++i) {
            Block block;
            block.itemId = noteAId;
            block.type = types.at(i);
            block.content = QStringLiteral("Line %1").arg(i);
            if (types.at(i) == QStringLiteral("todo")) {
                block.taskId = createdTask->id;
            }
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral("a Block could not be created: %1")
                                .arg(blocks.lastError()));
            }
            cascadeIds.append(created->id);
        }

        Block other;
        other.itemId = noteBId;
        other.type = QStringLiteral("text");
        other.content = otherContent;
        const std::optional<Block> createdOther = blocks.create(other);
        if (!createdOther) {
            return fail(QStringLiteral(
                            "the other Note's Block could not be created: %1")
                            .arg(blocks.lastError()));
        }
        otherId = createdOther->id;

        // A hard DELETE is the only operation that fires ON DELETE CASCADE;
        // the repository deliberately exposes no hard delete for Items.
        const QList<Block> beforeDeletion = blocks.readByItem(noteAId);
        if (!check(beforeDeletion.size() == cascadeIds.size(),
                   QStringLiteral(
                       "the owning Item's Blocks were not there to cascade"))) {
            return 1;
        }
        QSqlQuery deletion(database.connection());
        deletion.prepare(QStringLiteral("DELETE FROM items WHERE id = ?"));
        deletion.addBindValue(noteAId);
        if (!deletion.exec()) {
            return fail(QStringLiteral("the owning Item could not be deleted: %1")
                            .arg(deletion.lastError().text()));
        }
        if (!check(deletion.numRowsAffected() == 1,
                   QStringLiteral("the owning Item was not deleted"))) {
            return 1;
        }
    }

    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        BlockRepository blocks(database);

        if (!check(blocks.readByItem(noteAId).isEmpty(),
                   QStringLiteral(
                       "the deleted Item's Blocks remained after the restart"))) {
            return 1;
        }
        for (qlonglong id : cascadeIds) {
            if (!check(!blocks.readById(id),
                       QStringLiteral(
                           "an orphan Block row remained after the restart"))) {
                return 1;
            }
        }
        const QList<Block> other = blocks.readByItem(noteBId);
        if (!check(other.size() == 1 && other.first().id == otherId
                       && other.first().content == otherContent
                       && other.first().position == 0,
                   QStringLiteral(
                       "the cascade reached another Item's Blocks"))) {
            return 1;
        }
    }

    return 0;
}

// Issue 05 — the Phase 2 merge gate at the single high-level persistence seam.
// One representative set of Items of all three kinds (a Task, a Note and an
// Event), one soft-deleted Task, and Blocks of all five types belonging to that
// Note is written on a first launch. The connection is closed (the app restart),
// the same file is reopened through a fresh Database, and everything is read
// back unchanged through the public repository interfaces.
//
// The schema version is observed through Database::schemaVersion() — the public
// interface that reads the persisted meta row — rather than through a raw SQL
// side channel. A successful second open() shows the already-applied migration
// was skipped: re-running the versioned schema (plain CREATE TABLE, with no IF
// NOT EXISTS) against the existing tables would make open() fail. The version
// staying at the known literal and all data reading back unchanged show the
// schema was left intact and no table holding user data was rebuilt; writing in
// one session and reading in another shows no state lives only in memory.
int mergeGateRepresentativeSetSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    // Migration 1 (v0.1) is the one versioned schema in the single versioned
    // SQL source. This is an independent known literal, not recomputed here.
    const int expectedSchemaVersion = 1;

    // Independent, canonical literal values for the representative set.
    const QString taskTitle = QStringLiteral("Reply to Anna");
    const std::optional<QString> taskDueAt =
        QStringLiteral("2026-12-01T09:00:00Z");
    const std::optional<QString> taskDuePrecision = QStringLiteral("time");
    const std::optional<int> taskDurationMin = 45;
    const int taskPriority = 3;

    const QString noteTitle = QStringLiteral("Tuesday");
    const std::optional<QString> noteDay = QStringLiteral("2026-11-24");

    const QString eventTitle = QStringLiteral("Design review");
    const std::optional<QString> eventStartAt =
        QStringLiteral("2026-11-25T10:00:00Z");
    const std::optional<QString> eventEndAt =
        QStringLiteral("2026-11-25T11:00:00Z");
    const std::optional<int> eventDurationMin = 60;

    const QString archivedTaskTitle = QStringLiteral("Archived Task");

    // Five Block types in order, each with its own independent literal values.
    struct BlockSpec {
        QString type;
        QString content;
        std::optional<QString> inlineJson;
        std::optional<qlonglong> taskId;
        int indent = 0;
    };
    QList<BlockSpec> blockSpecs = {
        { QStringLiteral("text"),    QStringLiteral("A plain line"),
          std::nullopt, std::nullopt, 0 },
        { QStringLiteral("heading"), QStringLiteral("A heading"),
          std::nullopt, std::nullopt, 0 },
        { QStringLiteral("todo"),    QStringLiteral("A to-do line"),
          QStringLiteral("[{\"start\":0,\"end\":2,\"type\":\"italic\"}]"),
          std::nullopt, 1 },
        { QStringLiteral("bullet"),  QStringLiteral("A bullet point"),
          std::nullopt, std::nullopt, 2 },
        { QStringLiteral("code"),    QStringLiteral("int main() { return 0; }"),
          std::nullopt, std::nullopt, 0 },
    };

    qlonglong taskId = 0;
    qlonglong noteId = 0;
    qlonglong eventId = 0;
    qlonglong archivedTaskId = 0;
    QList<qlonglong> blockIds;

    // First launch: the schema and its meta version are created, then the whole
    // representative set is written through the repositories.
    {
        Database firstLaunch;
        if (!firstLaunch.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(firstLaunch.lastError()));
        }
        if (!check(firstLaunch.schemaVersion() == expectedSchemaVersion,
                   QStringLiteral(
                       "the versioned schema was not applied on first launch"))) {
            return 1;
        }
        if (!check(firstLaunch.lastError().isEmpty(),
                   QStringLiteral("first launch reported a diagnostic"))) {
            return 1;
        }
        const QString databasePath = Database::defaultDataDirectory()
            + QStringLiteral("/") + Database::databaseFileName();
        if (!check(QFile::exists(databasePath),
                   QStringLiteral(
                       "the single local SQLite file was not created on first "
                       "launch"))) {
            return 1;
        }

        ItemRepository items(firstLaunch);
        BlockRepository blocks(firstLaunch);

        Item task;
        task.kind = QStringLiteral("task");
        task.title = taskTitle;
        task.dueAt = taskDueAt;
        task.duePrecision = taskDuePrecision;
        task.durationMin = taskDurationMin;
        task.priority = taskPriority;
        task.sortOrder = 1;
        const std::optional<Item> createdTask = items.create(task);
        if (!createdTask) {
            return fail(QStringLiteral("the Task Item could not be created: %1")
                            .arg(items.lastError()));
        }
        taskId = createdTask->id;

        Item note;
        note.kind = QStringLiteral("note");
        note.title = noteTitle;
        note.day = noteDay;
        note.sortOrder = 2;
        const std::optional<Item> createdNote = items.create(note);
        if (!createdNote) {
            return fail(QStringLiteral("the Note Item could not be created: %1")
                            .arg(items.lastError()));
        }
        noteId = createdNote->id;

        Item event;
        event.kind = QStringLiteral("event");
        event.title = eventTitle;
        event.startAt = eventStartAt;
        event.endAt = eventEndAt;
        event.allDay = true;
        event.durationMin = eventDurationMin;
        event.priority = 1;
        event.sortOrder = 3;
        const std::optional<Item> createdEvent = items.create(event);
        if (!createdEvent) {
            return fail(QStringLiteral("the Event Item could not be created: %1")
                            .arg(items.lastError()));
        }
        eventId = createdEvent->id;

        // One more Task is soft-deleted so the merge gate can confirm the row is
        // hidden from normal reads but stays in the file across the restart.
        Item archivedTask;
        archivedTask.kind = QStringLiteral("task");
        archivedTask.title = archivedTaskTitle;
        archivedTask.sortOrder = 4;
        const std::optional<Item> createdArchived = items.create(archivedTask);
        if (!createdArchived) {
            return fail(QStringLiteral(
                            "the Task Item to soft-delete could not be created: %1")
                            .arg(items.lastError()));
        }
        archivedTaskId = createdArchived->id;
        if (!items.softDelete(archivedTaskId)) {
            return fail(QStringLiteral(
                            "the Task Item could not be soft-deleted: %1")
                            .arg(items.lastError()));
        }

        // A to-do Block points at the real Task Item it represents; a to-do
        // Block is a Block type, not a synonym for a Task.
        blockSpecs[2].taskId = taskId;
        for (const BlockSpec& spec : blockSpecs) {
            Block block;
            block.itemId = noteId;
            block.type = spec.type;
            block.content = spec.content;
            block.inlineJson = spec.inlineJson;
            block.taskId = spec.taskId;
            block.indent = spec.indent;
            const std::optional<Block> created = blocks.create(block);
            if (!created) {
                return fail(QStringLiteral(
                                "a Block of type '%1' could not be created: %2")
                                .arg(spec.type, blocks.lastError()));
            }
            blockIds.append(created->id);
        }
        // The Database destructor closes the connection: the app restart.
    }

    // Reopen through a fresh Database: this is the relaunch against the same
    // local SQLite file, with no in-memory state carried over.
    {
        Database afterRestart;
        // A successful reopen shows the already-applied migration was skipped:
        // re-running the versioned schema against the existing tables would
        // make open() fail. No table holding user data was rebuilt.
        if (!afterRestart.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(afterRestart.lastError()));
        }
        if (!check(afterRestart.lastError().isEmpty(),
                   QStringLiteral("the reopen reported a diagnostic"))) {
            return 1;
        }
        if (!check(afterRestart.schemaVersion() == expectedSchemaVersion,
                   QStringLiteral(
                       "the meta schema version did not survive the restart "
                       "unchanged"))) {
            return 1;
        }

        ItemRepository items(afterRestart);
        BlockRepository blocks(afterRestart);

        // Every Item of all three kinds reads back unchanged.
        const std::optional<Item> readTask = items.readById(taskId);
        if (!readTask) {
            return fail(QStringLiteral(
                            "the Task Item was not readable after the restart: %1")
                            .arg(items.lastError()));
        }
        if (!check(readTask->id == taskId
                       && readTask->kind == QStringLiteral("task")
                       && readTask->title == taskTitle
                       && readTask->dueAt == taskDueAt
                       && readTask->duePrecision == taskDuePrecision
                       && readTask->durationMin == taskDurationMin
                       && readTask->priority == taskPriority
                       && readTask->sortOrder == 1
                       && readTask->deletedAt == std::nullopt,
                   QStringLiteral(
                       "the Task Item did not survive the restart unchanged"))) {
            return 1;
        }

        const std::optional<Item> readNote = items.readById(noteId);
        if (!readNote) {
            return fail(QStringLiteral(
                            "the Note Item was not readable after the restart: %1")
                            .arg(items.lastError()));
        }
        if (!check(readNote->id == noteId
                       && readNote->kind == QStringLiteral("note")
                       && readNote->title == noteTitle
                       && readNote->day == noteDay
                       && readNote->sortOrder == 2,
                   QStringLiteral(
                       "the Note Item did not survive the restart unchanged"))) {
            return 1;
        }

        const std::optional<Item> readEvent = items.readById(eventId);
        if (!readEvent) {
            return fail(QStringLiteral(
                            "the Event Item was not readable after the restart: %1")
                            .arg(items.lastError()));
        }
        if (!check(readEvent->id == eventId
                       && readEvent->kind == QStringLiteral("event")
                       && readEvent->title == eventTitle
                       && readEvent->startAt == eventStartAt
                       && readEvent->endAt == eventEndAt
                       && readEvent->allDay == true
                       && readEvent->durationMin == eventDurationMin
                       && readEvent->priority == 1
                       && readEvent->sortOrder == 3,
                   QStringLiteral(
                       "the Event Item did not survive the restart unchanged"))) {
            return 1;
        }

        // All three kinds are readable by kind; the soft-deleted Task is
        // excluded from that normal read.
        if (!check(items.readByKind(QStringLiteral("task")).size() == 1,
                   QStringLiteral(
                       "reading Tasks by kind did not exclude the soft-deleted "
                       "Item"))) {
            return 1;
        }
        if (!check(items.readByKind(QStringLiteral("note")).size() == 1,
                   QStringLiteral("the Note Item did not read back by kind"))) {
            return 1;
        }
        if (!check(items.readByKind(QStringLiteral("event")).size() == 1,
                   QStringLiteral("the Event Item did not read back by kind"))) {
            return 1;
        }

        // The soft-deleted row is hidden from normal reads but remains in the
        // file, observed through the store-layer read (not a Trash UI).
        if (!check(!items.readById(archivedTaskId),
                   QStringLiteral(
                       "a soft-deleted Item was returned by a normal read"))) {
            return 1;
        }
        const std::optional<Item> storedArchived =
            items.readByIdIncludingSoftDeleted(archivedTaskId);
        if (!check(storedArchived.has_value(),
                   QStringLiteral(
                       "the soft-deleted row did not remain in the file"))) {
            return 1;
        }
        if (!check(storedArchived->title == archivedTaskTitle
                       && storedArchived->kind == QStringLiteral("task")
                       && storedArchived->deletedAt.has_value()
                       && QDateTime::fromString(*storedArchived->deletedAt,
                                                Qt::ISODate).isValid(),
                   QStringLiteral(
                       "the soft-deleted Item's fields or deleted_at changed"))) {
            return 1;
        }

        // All five Block types read back in order with every field unchanged.
        const QList<Block> read = blocks.readByItem(noteId);
        if (!check(read.size() == blockSpecs.size(),
                   QStringLiteral(
                       "the Note's Blocks did not all survive the restart"))) {
            return 1;
        }
        for (int i = 0; i < blockSpecs.size(); ++i) {
            const Block& block = read.at(i);
            const BlockSpec& spec = blockSpecs.at(i);
            if (!check(block.id == blockIds.at(i)
                           && block.itemId == noteId
                           && block.position == i
                           && block.type == spec.type
                           && block.content == spec.content
                           && block.inlineJson == spec.inlineJson
                           && block.taskId == spec.taskId
                           && block.indent == spec.indent,
                       QStringLiteral(
                           "a Block did not survive the restart unchanged"))) {
                return 1;
            }
        }
    }

    return 0;
}

// Regression: a default-constructed (null) QString title must not be bound as
// SQL NULL. items.title is NOT NULL DEFAULT '', but every writable column is
// bound explicitly, so the schema default never applies and a null title would
// violate the constraint. An Item created without assigning a title is stored
// as the empty string, round-trips through a restart, and stays updatable with
// a null title — exercising both the create() and update() binding paths.
int untitledItemRoundTripsThroughCreateAndUpdate()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString kind = QStringLiteral("task");
    qlonglong id = 0;
    Item written;

    // First session: create an Item with no title assigned, then close the
    // connection (the app restart).
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be opened on first launch: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        Item item;
        item.kind = kind;
        // Deliberately leave item.title unassigned: a default-constructed
        // QString is null, not empty.
        if (!check(item.title.isNull(),
                   QStringLiteral(
                       "the fixture Item's title is not a null QString"))) {
            return 1;
        }

        const std::optional<Item> created = items.create(item);
        if (!created) {
            return fail(QStringLiteral(
                            "an untitled Item could not be created: %1")
                            .arg(items.lastError()));
        }
        written = *created;
    }

    if (!check(written.id > 0,
               QStringLiteral("the untitled Item has no assigned id"))) {
        return 1;
    }
    id = written.id;
    // The returned Item matches the stored value: a non-null, empty title
    // (not a null QString that would disagree with a read-back).
    if (!check(!written.title.isNull() && written.title.isEmpty(),
               QStringLiteral(
                   "create returned a null or non-empty title rather than the "
                   "stored empty string"))) {
        return 1;
    }

    // Second session: reopen and read the Item back, then update it with a
    // null title to exercise the shared binding path in update() too.
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "restart: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const std::optional<Item> read = items.readById(id);
        if (!read) {
            return fail(QStringLiteral(
                            "the untitled Item was not readable after the "
                            "restart: %1")
                            .arg(items.lastError()));
        }
        if (!check(read->id == id && read->kind == kind,
                   QStringLiteral(
                       "the untitled Item's id or kind changed"))) {
            return 1;
        }
        if (!check(!read->title.isNull() && read->title.isEmpty(),
                   QStringLiteral(
                       "the untitled Item's title did not round-trip as a "
                       "non-null empty string"))) {
            return 1;
        }

        Item revised = *read;
        revised.title = QString();   // explicitly null again
        revised.priority = 2;
        if (!check(revised.title.isNull(),
                   QStringLiteral(
                       "the update fixture's title is not a null QString"))) {
            return 1;
        }
        if (!items.update(revised)) {
            return fail(QStringLiteral(
                            "an untitled Item could not be updated: %1")
                            .arg(items.lastError()));
        }
    }

    // Third session: after another restart, the empty-string title and the
    // changed priority both survive the null-title update.
    {
        Database database;
        if (!database.open()) {
            return fail(QStringLiteral(
                            "the database could not be reopened after the "
                            "update: %1")
                            .arg(database.lastError()));
        }
        ItemRepository items(database);

        const std::optional<Item> read = items.readById(id);
        if (!read) {
            return fail(QStringLiteral(
                            "the updated untitled Item was not readable: %1")
                            .arg(items.lastError()));
        }
        if (!check(!read->title.isNull() && read->title.isEmpty()
                       && read->priority == 2,
                   QStringLiteral(
                       "the untitled Item did not survive a null-title "
                       "update"))) {
            return 1;
        }
    }

    return 0;
}

// Code-review finding: two live Database instances must not share one Qt SQL
// connection name. With a shared name, opening the second silently replaces
// (and invalidates) the first's connection via QSqlDatabase::addDatabase(),
// and a close() can remove a connection another instance still holds. Each
// instance now registers and removes only its own uniquely named connection.
// Overlapping lifetimes are exercised together with closure in BOTH orders:
// closing the first then using the second, and closing the second then using
// the first. All assertions go through the public repository API against the
// one local SQLite file, never a side channel.
int overlappingDatabasesKeepIndependentConnections()
{
    // Closure order 1: close the first instance, keep using the second.
    {
        QTemporaryDir dataDir;
        if (freshAppData(&dataDir) != 0) {
            return 1;
        }

        Database first;
        Database second;
        if (!first.open()) {
            return fail(QStringLiteral(
                            "the first Database could not be opened: %1")
                            .arg(first.lastError()));
        }
        if (!second.open()) {
            return fail(QStringLiteral(
                            "a second Database could not be opened while the "
                            "first was live: %1")
                            .arg(second.lastError()));
        }

        // Opening the second must not have replaced the first's connection.
        if (!check(first.connection().isValid() && first.connection().isOpen(),
                   QStringLiteral(
                       "opening a second Database invalidated the first's "
                       "connection"))) {
            return 1;
        }
        if (!check(second.connection().isValid() && second.connection().isOpen(),
                   QStringLiteral(
                       "the second Database's connection is not open"))) {
            return 1;
        }
        if (!check(first.connection().connectionName()
                       != second.connection().connectionName(),
                   QStringLiteral(
                       "two live Database instances share one connection "
                       "name"))) {
            return 1;
        }

        ItemRepository itemsFirst(first);
        ItemRepository itemsSecond(second);

        // Write through the second, read through the first: the first's
        // connection still works after the second opened, and both see the
        // same local file.
        Item seed;
        seed.kind = QStringLiteral("task");
        seed.title = QStringLiteral("Shared-file probe");
        const std::optional<Item> written = itemsSecond.create(seed);
        if (!written) {
            return fail(QStringLiteral(
                            "the second Database could not write an Item: %1")
                            .arg(itemsSecond.lastError()));
        }
        const std::optional<Item> viaFirst = itemsFirst.readById(written->id);
        if (!check(viaFirst && viaFirst->title == seed.title,
                   QStringLiteral(
                       "the first Database stopped working when the second "
                       "opened"))) {
            return 1;
        }

        // Closing the first must not remove the second's connection.
        first.close();
        const std::optional<Item> afterFirstClose =
            itemsSecond.readById(written->id);
        if (!check(afterFirstClose && afterFirstClose->title == seed.title,
                   QStringLiteral(
                       "closing the first Database removed the second's "
                       "connection"))) {
            return 1;
        }
        Item afterCloseItem;
        afterCloseItem.kind = QStringLiteral("note");
        afterCloseItem.title = QStringLiteral("Written after the first closed");
        if (!itemsSecond.create(afterCloseItem)) {
            return fail(QStringLiteral(
                            "the second Database could not write after the "
                            "first closed: %1")
                            .arg(itemsSecond.lastError()));
        }
    }

    // Closure order 2: close the second instance, keep using the first.
    {
        QTemporaryDir dataDir;
        if (freshAppData(&dataDir) != 0) {
            return 1;
        }

        Database first;
        Database second;
        if (!first.open()) {
            return fail(QStringLiteral(
                            "the first Database could not be opened: %1")
                            .arg(first.lastError()));
        }
        if (!second.open()) {
            return fail(QStringLiteral(
                            "a second Database could not be opened while the "
                            "first was live: %1")
                            .arg(second.lastError()));
        }

        ItemRepository itemsFirst(first);
        ItemRepository itemsSecond(second);

        Item seed;
        seed.kind = QStringLiteral("task");
        seed.title = QStringLiteral("Shared-file probe, second order");
        const std::optional<Item> written = itemsFirst.create(seed);
        if (!written) {
            return fail(QStringLiteral(
                            "the first Database could not write an Item: %1")
                            .arg(itemsFirst.lastError()));
        }
        const std::optional<Item> viaSecond = itemsSecond.readById(written->id);
        if (!check(viaSecond && viaSecond->title == seed.title,
                   QStringLiteral(
                       "the second Database could not read the first's Item "
                       "while both were live"))) {
            return 1;
        }

        // Closing the second must not remove the first's connection.
        second.close();
        const std::optional<Item> afterSecondClose =
            itemsFirst.readById(written->id);
        if (!check(afterSecondClose && afterSecondClose->title == seed.title,
                   QStringLiteral(
                       "closing the second Database removed the first's "
                       "connection"))) {
            return 1;
        }
        Item afterCloseItem;
        afterCloseItem.kind = QStringLiteral("note");
        afterCloseItem.title = QStringLiteral("Written after the second closed");
        if (!itemsFirst.create(afterCloseItem)) {
            return fail(QStringLiteral(
                            "the first Database could not write after the "
                            "second closed: %1")
                            .arg(itemsFirst.lastError()));
        }
    }

    return 0;
}

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

    // Issue 02, the original tracer bullet: one Task Item survives a restart.
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

    if (todayDataSurvivesRestartTogether() != 0) {
        return 1;
    }
    if (todayTaskWriteSurvivesRestart() != 0) {
        return 1;
    }
    if (todayCompletionSurvivesRestart() != 0) {
        return 1;
    }
    if (todayCompletionRefreshesAlreadyStoredState() != 0) {
        return 1;
    }
    if (richNoteEditIsSafe() != 0) {
        return 1;
    }
    if (dayNoteWritingSurvivesRestart() != 0) {
        return 1;
    }
    if (fullItemColumnsSurviveRestart() != 0) {
        return 1;
    }
    if (untitledItemRoundTripsThroughCreateAndUpdate() != 0) {
        return 1;
    }
    if (itemsOfAllKindsAndQueriesSurviveRestart() != 0) {
        return 1;
    }
    if (folderAndTagQueriesSurviveRestart() != 0) {
        return 1;
    }
    if (updatesSurviveRestart() != 0) {
        return 1;
    }
    if (softDeleteSurvivesRestart() != 0) {
        return 1;
    }
    if (constraintViolationsAreRejectedClearly() != 0) {
        return 1;
    }
    if (deletedDayNoteDateCanBeReused() != 0) {
        return 1;
    }
    if (aBlockSurvivesRestart() != 0) {
        return 1;
    }
    if (blocksOfAllTypesSurviveRestart() != 0) {
        return 1;
    }
    if (blockUpdatesSurviveRestart() != 0) {
        return 1;
    }
    if (blockUpdatePreservesOwnerAndPosition() != 0) {
        return 1;
    }
    if (blockDeletionResequencesAndSurvivesRestart() != 0) {
        return 1;
    }
    if (blockReorderSurvivesRestart() != 0) {
        return 1;
    }
    if (blockInsertionResequencesAndSurvivesRestart() != 0) {
        return 1;
    }
    if (deletingAnItemCascadesToItsBlocks() != 0) {
        return 1;
    }
    if (mergeGateRepresentativeSetSurvivesRestart() != 0) {
        return 1;
    }
    if (overlappingDatabasesKeepIndependentConnections() != 0) {
        return 1;
    }

    std::puts("verify-persistence: PASS - the Phase 2 merge gate holds: one "
              "representative set of Items of every kind and Blocks of every "
              "type, with every field, query shape, update, soft-delete, Block "
              "order, cascade and constraint error, survives a restart "
              "unchanged; the schema is created on first launch, left intact on "
              "reopen, and the persisted schema version survives; two live "
              "Database instances keep independent connections and survive "
              "closure in either order");
    return 0;
}
