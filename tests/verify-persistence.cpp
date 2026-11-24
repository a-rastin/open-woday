// The single high-level persistence seam of Phase 2 (issues 02 and 03): Items
// are written through the store layer, the Database connection is closed and
// reopened against the same local SQLite file, and the Items are read back
// through the same public API — with every column, kind, query shape, update,
// soft-delete and constraint error intact.
//
// This is an executable specification, not a test framework: no QtTest, no
// catch/gtest, no CI configuration, no mocks, no assertions about SQL strings
// or private helpers. It observes only externally visible Item values.
// Folders and Tags have no repository in this phase, so their few fixture
// rows are seeded with bound values; every assertion still reads through
// ItemRepository.

#include <QCoreApplication>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QVariant>
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

    if (fullItemColumnsSurviveRestart() != 0) {
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

    std::puts("verify-persistence: PASS - Items of every kind, with every "
              "field, update, soft-delete, query shape and constraint error, "
              "survive a restart unchanged");
    return 0;
}
