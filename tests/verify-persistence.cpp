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
#include <QEventLoop>
#include <QGuiApplication>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>
#include <QVariant>
#include <QtGlobal>

#include <cstdio>
#include <optional>
#include <sqlite3.h>

#include "store/Database.h"
#include "store/BlockRepository.h"
#include "store/FolderRepository.h"
#include "store/ItemRepository.h"
#include "store/TagRepository.h"
#include "app/ItemModel.h"
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

int midnightRollover()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    QDateTime now(QDate(2026, 1, 1), QTime(23, 59));
    TodayModel model(database, nullptr, [&now] { return now; });
    ItemRepository items(database);
    BlockRepository blocks(database);
    const auto oldNote = items.readByDay(QStringLiteral("2026-01-01")).first();
    model.setDayNoteText(QStringLiteral("before midnight"));
    int resets = 0;
    QObject::connect(&model, &QAbstractItemModel::modelReset, [&resets] { ++resets; });
    now = now.addDays(1);
    QMetaObject::invokeMethod(&model, "updateToday");
    const auto saved = blocks.readByItem(oldNote.id);
    if (!check(resets == 1 && model.selectedDate() == QStringLiteral("2026-01-02")
               && model.data(model.index(4), TodayModel::IsTodayRole).toBool()
               && model.data(model.index(4), TodayModel::DateRole).toString() == model.selectedDate()
               && saved.size() == 1 && saved.first().content == QStringLiteral("before midnight"),
               QStringLiteral("midnight did not refresh today and save the previous note")))
        return 1;
    model.selectDate(QStringLiteral("2026-01-01"));
    now = now.addDays(1);
    QMetaObject::invokeMethod(&model, "updateToday");
    if (!check(resets == 2 && model.selectedDate() == QStringLiteral("2026-01-01")
               && model.data(model.index(4), TodayModel::DateRole).toString() == QStringLiteral("2026-01-03")
               && model.dayNoteText() == QStringLiteral("before midnight"),
               QStringLiteral("midnight changed an independently selected date")))
        return 1;
    model.selectDate(QStringLiteral("2026-01-03"));
    model.setDayNoteText(QStringLiteral("unsaved"));
    QSqlQuery query(database.connection());
    if (!query.exec(QStringLiteral(
            "CREATE TEMP TRIGGER fail_midnight_save BEFORE INSERT ON blocks "
            "BEGIN SELECT RAISE(FAIL, 'save failed'); END")))
        return fail(query.lastError().text());
    now = now.addDays(1);
    QMetaObject::invokeMethod(&model, "updateToday");
    if (!check(resets == 3 && model.selectedDate() == QStringLiteral("2026-01-03")
               && model.dayNoteText() == QStringLiteral("unsaved")
               && !model.dayNoteError().isEmpty()
               && model.data(model.index(4), TodayModel::DateRole).toString() == QStringLiteral("2026-01-04"),
               QStringLiteral("failed midnight save did not preserve the selected note")))
        return 1;
    if (!query.exec(QStringLiteral("DROP TRIGGER fail_midnight_save")) || !model.flushDayNote())
        return fail(QStringLiteral("could not save note after simulated failure"));

    const QString earliest = model.data(model.index(0), TodayModel::DateRole).toString();
    model.selectDate(earliest);
    const auto earliestNote = items.readByDay(earliest).first();
    model.setDayNoteText(QStringLiteral("earliest note"));
    now = now.addDays(1);
    QMetaObject::invokeMethod(&model, "updateToday");
    const auto earliestSaved = blocks.readByItem(earliestNote.id);
    bool selectedVisible = false;
    for (int i = 0; i < model.rowCount(); ++i)
        selectedVisible |= model.data(model.index(i), TodayModel::IsSelectedRole).toBool();
    if (!check(selectedVisible && model.selectedDate() != earliest
               && earliestSaved.size() == 1
               && earliestSaved.first().content == QStringLiteral("earliest note"),
               QStringLiteral("midnight stranded the earliest selection or did not save its note")))
        return 1;

    const QString failingDate = model.data(model.index(0), TodayModel::DateRole).toString();
    model.selectDate(failingDate);
    model.setDayNoteText(QStringLiteral("unsaved earliest note"));
    if (!query.exec(QStringLiteral(
            "CREATE TEMP TRIGGER fail_earliest_save BEFORE UPDATE ON blocks "
            "BEGIN SELECT RAISE(FAIL, 'save failed'); END")))
        return fail(query.lastError().text());
    now = now.addDays(1);
    QMetaObject::invokeMethod(&model, "updateToday");
    selectedVisible = false;
    for (int i = 0; i < model.rowCount(); ++i)
        selectedVisible |= model.data(model.index(i), TodayModel::IsSelectedRole).toBool()
            && model.data(model.index(i), TodayModel::DateRole).toString() == failingDate;
    if (!check(selectedVisible && model.selectedDate() == failingDate
               && model.dayNoteText() == QStringLiteral("unsaved earliest note")
               && !model.dayNoteError().isEmpty(),
               QStringLiteral("failed earliest save stranded or lost the selected note")))
        return 1;
    if (!query.exec(QStringLiteral("DROP TRIGGER fail_earliest_save")) || !model.flushDayNote())
        return fail(QStringLiteral("could not save earliest note after simulated failure"));
    return 0;
}

int todayClockChecks()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());

    QDateTime now(QDate(2026, 1, 1), QTime(23, 59, 59, 950));
    TodayModel model(database, nullptr, [&now] { return now; });
    now = now.addMSecs(100);
    QEventLoop loop;
    QTimer::singleShot(250, &loop, &QEventLoop::quit);
    loop.exec();
    if (!check(model.selectedDate() == QStringLiteral("2026-01-02"),
               QStringLiteral("midnight timer did not refresh today")))
        return 1;

    now = now.addDays(-2);
    auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    app->applicationStateChanged(Qt::ApplicationInactive);
    if (!check(model.selectedDate() == QStringLiteral("2026-01-02"),
               QStringLiteral("inactive state unexpectedly refreshed today")))
        return 1;
    app->applicationStateChanged(Qt::ApplicationActive);
    if (!check(model.selectedDate() == QStringLiteral("2025-12-31"),
               QStringLiteral("activation did not refresh today after a clock change")))
        return 1;
    return 0;
}

int debouncedDayNoteWrites()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    TodayModel model(database);
    ItemRepository items(database);
    BlockRepository blocks(database);
    const auto notes = items.readByDay(model.selectedDate());
    if (!check(notes.size() == 1, QStringLiteral("missing Day note")))
        return 1;
    const auto id = notes.first().id;
    model.setDayNoteText(QStringLiteral("first\nsecond"));
    if (!check(blocks.readByItem(id).isEmpty(), QStringLiteral("note saved before debounce")))
        return 1;
    auto wait = [](int ms) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    };
    wait(250);
    model.setDayNoteText(QStringLiteral("first\nchanged"));
    wait(250);
    if (!check(blocks.readByItem(id).isEmpty(), QStringLiteral("debounce was not restarted")))
        return 1;
    wait(300);
    auto stored = blocks.readByItem(id);
    if (!check(stored.size() == 2 && stored.at(1).content == QStringLiteral("changed"),
               QStringLiteral("debounced save did not persist latest text")))
        return 1;
    QSqlQuery query(database.connection());
    if (!query.exec(QStringLiteral("CREATE TEMP TABLE block_updates (id INTEGER)"))
        || !query.exec(QStringLiteral("CREATE TEMP TRIGGER count_block_updates AFTER UPDATE ON blocks "
                                      "BEGIN INSERT INTO block_updates VALUES (NEW.id); END")))
        return fail(query.lastError().text());
    model.setDayNoteText(QStringLiteral("first\nfinal"));
    if (!check(model.flushDayNote(), QStringLiteral("explicit flush failed")))
        return 1;
    if (!query.exec(QStringLiteral("SELECT id FROM block_updates")) || !query.next())
        return fail(QStringLiteral("changed line was not updated"));
    if (!check(query.value(0).toLongLong() == stored.at(1).id && !query.next(),
               QStringLiteral("unchanged lines were updated")))
        return 1;
    model.setDayNoteText(QStringLiteral("temporary"));
    model.setDayNoteText(QStringLiteral("first\nfinal"));
    wait(500);
    stored = blocks.readByItem(id);
    if (!check(stored.size() == 2 && stored.at(1).content == QStringLiteral("final"),
               QStringLiteral("reverting pending text changed persisted note")))
        return 1;
    return 0;
}

// Replacement uses ordered row identities, not stored positions, even when
// positions were written outside the repository's usual contiguous sequence.
int replaceTextWithNoncontiguousPositions()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    ItemRepository items(database);
    const auto note = items.create(QStringLiteral("note"), QStringLiteral("Gap note"));
    const auto other = items.create(QStringLiteral("note"), QStringLiteral("Other note"));
    if (!note || !other)
        return fail(items.lastError());
    BlockRepository blocks(database);
    QList<qlonglong> ids;
    for (int i = 0; i < 3; ++i) {
        Block block;
        block.itemId = note->id;
        block.type = QStringLiteral("text");
        block.content = QString::number(i);
        const auto created = blocks.create(block);
        if (!created)
            return fail(blocks.lastError());
        ids.append(created->id);
        QSqlQuery seed(database.connection());
        seed.prepare(QStringLiteral("UPDATE blocks SET position = ? WHERE id = ?"));
        seed.addBindValue(10 + i * 10);
        seed.addBindValue(created->id);
        if (!seed.exec())
            return fail(seed.lastError().text());
    }
    Block foreign;
    foreign.itemId = other->id;
    foreign.type = QStringLiteral("text");
    foreign.content = QStringLiteral("untouched");
    const auto foreignCreated = blocks.create(foreign);
    if (!foreignCreated)
        return fail(blocks.lastError());

    if (!blocks.replaceText(note->id, QStringLiteral("first\nsecond")))
        return fail(blocks.lastError());
    QList<Block> result = blocks.readByItem(note->id);
    if (!check(result.size() == 2 && result.at(0).id == ids.at(0)
                   && result.at(1).id == ids.at(1)
                   && result.at(0).position == 0 && result.at(1).position == 1
                   && result.at(0).content == QStringLiteral("first")
                   && result.at(1).content == QStringLiteral("second")
                   && !blocks.readById(ids.at(2)),
               QStringLiteral("shrinking a note with position gaps lost or retained the wrong blocks")))
        return 1;

    QSqlQuery seed(database.connection());
    seed.prepare(QStringLiteral("UPDATE blocks SET position = position + 10 WHERE item_id = ?"));
    seed.addBindValue(note->id);
    if (!seed.exec())
        return fail(seed.lastError().text());
    if (!blocks.replaceText(note->id, QStringLiteral("a\nb\nc\nd")))
        return fail(blocks.lastError());
    result = blocks.readByItem(note->id);
    if (!check(result.size() == 4, QStringLiteral("expanding a note with position gaps lost blocks")))
        return 1;
    const QStringList expected{QStringLiteral("a"), QStringLiteral("b"),
                               QStringLiteral("c"), QStringLiteral("d")};
    for (int i = 0; i < result.size(); ++i) {
        if (!check(result.at(i).position == i && result.at(i).content == expected.at(i)
                       && (i >= 2 || result.at(i).id == ids.at(i)),
                   QStringLiteral("expanded note has incorrect block order, identity or content")))
            return 1;
    }
    const auto foreignAfter = blocks.readById(foreignCreated->id);
    if (!check(foreignAfter && foreignAfter->content == foreign.content
                   && foreignAfter->position == foreignCreated->position,
               QStringLiteral("replacement modified another note")))
        return 1;
    return 0;
}

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

int todayQueriesOnlySelectedDay()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    ItemRepository items(database);
    const QString day = QDate::currentDate().toString(Qt::ISODate);
    const QString next = QDate::currentDate().addDays(1).toString(Qt::ISODate);
    auto create = [&](const QString& kind, const QString& date, int order,
                      bool deleted = false) {
        Item item;
        item.kind = kind;
        item.title = date;
        item.sortOrder = order;
        if (!date.isNull()) {
            if (kind == QStringLiteral("task"))
                item.dueAt = date;
            else
                item.startAt = date;
        }
        if (deleted)
            item.deletedAt = QStringLiteral("2026-01-01T00:00:00Z");
        return items.create(item);
    };
    const auto taskTime = create(QStringLiteral("task"), day + QStringLiteral("T23:59:59"), 2);
    const auto taskDay = create(QStringLiteral("task"), day, 1);
    const auto eventTime = create(QStringLiteral("event"), day + QStringLiteral("T00:00:00"), 2);
    const auto eventDay = create(QStringLiteral("event"), day, 1);
    if (!taskTime || !taskDay || !eventTime || !eventDay
        || !create(QStringLiteral("task"), next, 0)
        || !create(QStringLiteral("event"), next, 0)
        || !create(QStringLiteral("task"), QString(), 0)
        || !create(QStringLiteral("event"), QString(), 0)
        || !create(QStringLiteral("task"), day, 0, true)
        || !create(QStringLiteral("event"), day, 0, true))
        return fail(items.lastError());
    const QList<Item> tasks = items.readTasksDueOn(day);
    const QList<Item> events = items.readEventsStartingOn(day);
    if (!check(tasks.size() == 2 && tasks.at(0).id == taskDay->id
                   && tasks.at(1).id == taskTime->id
                   && events.size() == 2 && events.at(0).id == eventDay->id
                   && events.at(1).id == eventTime->id,
               QStringLiteral("day-scoped Item queries returned incorrect rows or order")))
        return 1;
    TodayModel model(database);
    const QVariantList lines = model.lines();
    if (!check(lines.size() == 4 && model.taskCount() == 2 && model.eventCount() == 2
                   && lines.at(0).toMap().value("id").toString() == QString::number(taskDay->id)
                   && lines.at(2).toMap().value("id").toString() == QString::number(eventDay->id),
               QStringLiteral("Today did not display the selected day's Items in order")))
        return 1;
    model.selectDate(next);
    if (!check(model.taskCount() == 1 && model.eventCount() == 1 && model.lines().size() == 2,
               QStringLiteral("Today did not refresh for the newly selected day")))
        return 1;

    // These instants belong to today locally, but are written on adjacent days.
    const QDateTime early(QDate::currentDate(), QTime(0, 30));
    const QDateTime late(QDate::currentDate(), QTime(23, 30));
    const QString previousWritten = early.toOffsetFromUtc(early.offsetFromUtc() - 3600)
                                        .toString(Qt::ISODate);
    const QString nextWritten = late.toOffsetFromUtc(late.offsetFromUtc() + 3600)
                                    .toString(Qt::ISODate);
    const auto shiftedTask = create(QStringLiteral("task"), previousWritten, 3);
    const auto shiftedEvent = create(QStringLiteral("event"), nextWritten, 3);
    const QDateTime nextEarly(QDate::currentDate().addDays(1), QTime(0, 30));
    const QString misleading = nextEarly.toOffsetFromUtc(nextEarly.offsetFromUtc() - 3600)
                                   .toString(Qt::ISODateWithMs);
    const auto nextTask = create(QStringLiteral("task"), misleading, 4);
    const auto invalidEvent = create(QStringLiteral("event"), day + QStringLiteral("Tbad"), 4);
    const auto utcTask = create(QStringLiteral("task"), early.toUTC().toString(Qt::ISODate), 5);
    if (!shiftedTask || !shiftedEvent || !nextTask || !invalidEvent || !utcTask)
        return fail(items.lastError());
    const auto hasId = [](const QList<Item>& rows, qlonglong id) {
        for (const Item& row : rows)
            if (row.id == id)
                return true;
        return false;
    };
    if (!check(hasId(items.readTasksDueOn(day), shiftedTask->id)
                   && hasId(items.readEventsStartingOn(day), shiftedEvent->id)
                   && hasId(items.readTasksDueOn(day), utcTask->id)
                   && !hasId(items.readTasksDueOn(day), nextTask->id)
                   && hasId(items.readTasksDueOn(next), nextTask->id)
                   && !hasId(items.readEventsStartingOn(day), invalidEvent->id)
                   && !hasId(items.readEventsStartingOn(next), shiftedEvent->id),
               QStringLiteral("offset timestamps were not matched to their local day")))
        return 1;
    model.selectDate(day);
    if (!check(model.taskCount() == 4 && model.eventCount() == 3,
               QStringLiteral("Today did not show offset timestamps on their local day")))
        return 1;
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

// Deny a particular SELECT after the task lookup, simulating either failed
// repository read without disrupting the preceding successful write/lookup.
int denySelectedRead(void* context, int action, const char*, const char*,
                     const char*, const char*)
{
    int* remaining = static_cast<int*>(context);
    if (action == SQLITE_SELECT && --*remaining == 0)
        return SQLITE_DENY;
    return SQLITE_OK;
}

int todayRefreshRetainsStateOnReadFailure()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    ItemRepository items(database);
    Item event;
    event.kind = QStringLiteral("event");
    event.title = QStringLiteral("Existing event");
    event.startAt = QDate::currentDate().toString(Qt::ISODate) + QStringLiteral("T10:00:00");
    if (!items.create(event))
        return fail(items.lastError());
    TodayModel model(database);
    if (!model.addTask(QStringLiteral("Existing task")))
        return fail(QStringLiteral("could not create fixture Task"));
    const QVariantList original = model.lines();
    const QString id = original.first().toMap().value("id").toString();
    const QString summary = model.summary();
    if (!check(original.size() == 2 && model.taskCount() == 1 && model.eventCount() == 1,
               QStringLiteral("Task and Event fixture missing")))
        return 1;
    QVariant handle = database.connection().driver()->handle();
    if (!check(handle.isValid() && qstrcmp(handle.typeName(), "sqlite3*") == 0,
               QStringLiteral("SQLite handle unavailable")))
        return 1;
    sqlite3* sqlite = *static_cast<sqlite3**>(handle.data());
    for (int read = 2; read <= 3; ++read) {
        int remaining = read;
        sqlite3_set_authorizer(sqlite, denySelectedRead, &remaining);
        model.setTaskCompleted(id, false);
        sqlite3_set_authorizer(sqlite, nullptr, nullptr);
        if (!check(remaining == 0 && model.lines() == original
                       && model.summary() == summary
                       && model.taskCount() == 1 && model.eventCount() == 1
                       && !model.property("refreshError").toString().isEmpty(),
                   QStringLiteral("failed Item read replaced state or hid its error")))
            return 1;
        model.setTaskCompleted(id, false);
        if (!check(model.property("refreshError").toString().isEmpty()
                       && model.lines() == original,
                   QStringLiteral("successful refresh did not clear its error")))
            return 1;
    }
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
    if (!check(!model.property("dayNoteEditable").toBool()
                   && !model.dayNoteError().isEmpty(),
               QStringLiteral("formatted Day note was not marked read-only")))
        return 1;
    const QString noteError = model.dayNoteError();
    int taskErrorChanges = 0;
    QObject::connect(&model, &TodayModel::taskErrorChanged,
                     [&taskErrorChanges] { ++taskErrorChanges; });
    QSqlQuery query(database.connection());
    if (!query.exec(QStringLiteral(
            "CREATE TRIGGER reject_task_insert BEFORE INSERT ON items "
            "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'task insert failed'); END")))
        return fail(query.lastError().text());
    if (!check(!model.addTask(QStringLiteral("Task"))
                   && model.property("taskError").toString().contains(QStringLiteral("task insert failed"))
                   && model.dayNoteError() == noteError && taskErrorChanges == 1,
               QStringLiteral("task creation error did not preserve the note explanation")))
        return 1;
    if (!query.exec(QStringLiteral("DROP TRIGGER reject_task_insert")))
        return fail(query.lastError().text());
    if (!check(model.addTask(QStringLiteral("Task")) && model.taskError().isEmpty()
                   && model.dayNoteError() == noteError && taskErrorChanges == 2,
               QStringLiteral("successful task creation did not clear its error")))
        return 1;
    const QString taskId = model.lines().first().toMap().value("id").toString();
    if (!query.exec(QStringLiteral(
            "CREATE TRIGGER reject_task_update BEFORE UPDATE ON items "
            "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'task update failed'); END")))
        return fail(query.lastError().text());
    if (!check(!model.setTaskCompleted(taskId, true)
                   && model.taskError().contains(QStringLiteral("task update failed"))
                   && model.dayNoteError() == noteError,
               QStringLiteral("task update error did not preserve the note explanation")))
        return 1;
    if (!query.exec(QStringLiteral("DROP TRIGGER reject_task_update")))
        return fail(query.lastError().text());
    if (!check(model.setTaskCompleted(taskId, false) && model.taskError().isEmpty()
                   && model.dayNoteError() == noteError,
               QStringLiteral("unchanged task completion did not clear its error")))
        return 1;
    if (!query.exec(QStringLiteral(
            "CREATE TRIGGER reject_task_update BEFORE UPDATE ON items "
            "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'task update failed'); END")))
        return fail(query.lastError().text());
    if (!model.setTaskCompleted(taskId, true) && !model.taskError().isEmpty()) {
        if (!query.exec(QStringLiteral("DROP TRIGGER reject_task_update")))
            return fail(query.lastError().text());
    } else {
        return fail(QStringLiteral("task update failure fixture did not fail"));
    }
    if (!check(model.setTaskCompleted(taskId, true) && model.taskError().isEmpty()
                   && model.dayNoteError() == noteError,
               QStringLiteral("successful task completion did not clear its error")))
        return 1;
    model.setDayNoteText(QStringLiteral("Changed"));
    model.selectDate(QDate::currentDate().addDays(1).toString(Qt::ISODate));
    const auto after = blocks.readById(stored->id);
    if (!check(after && after->type == heading.type && after->content == heading.content
                   && model.selectedDate() != *note.day
                   && model.dayNoteText().isEmpty()
                   && model.property("dayNoteEditable").toBool(),
               QStringLiteral("formatted note blocked navigation or was changed")))
        return 1;
    model.selectDate(*note.day);
    if (!check(!model.property("dayNoteEditable").toBool()
                   && model.dayNoteText() == heading.content,
               QStringLiteral("formatted note did not return to read-only")))
        return 1;
    return 0;
}

int dayNoteSaveFailureBlocksNavigationWithExplanation()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;
    Database database;
    if (!database.open())
        return fail(database.lastError());
    TodayModel model(database);
    const QString today = model.selectedDate();
    const QString tomorrow = QDate::currentDate().addDays(1).toString(Qt::ISODate);
    QSqlQuery query(database.connection());
    if (!query.exec(QStringLiteral(
            "CREATE TRIGGER reject_note BEFORE INSERT ON blocks "
            "BEGIN SELECT RAISE(FAIL, 'temporary failure'); END")))
        return fail(QStringLiteral("could not create save failure fixture"));
    model.setDayNoteText(QStringLiteral("Unsaved"));
    model.selectDate(tomorrow);
    if (!check(model.selectedDate() == today
                   && model.dayNoteText() == QStringLiteral("Unsaved")
                   && model.dayNoteError().contains(QStringLiteral("Cannot change date"))
                   && model.dayNoteError().contains(QStringLiteral("temporary failure")),
               QStringLiteral("failed save did not explain refused navigation")))
        return 1;
    if (!query.exec(QStringLiteral("DROP TRIGGER reject_note")))
        return fail(QStringLiteral("could not remove save failure fixture"));
    model.selectDate(tomorrow);
    model.selectDate(today);
    if (!check(model.dayNoteText() == QStringLiteral("Unsaved")
                   && model.dayNoteError().isEmpty(),
               QStringLiteral("retry did not save the Day note")))
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

// Issue 03, slice 5 (updated in Phase 5, ticket 05): soft-delete survives a
// restart and atomically cascades deleted_at to all active descendant subtasks
// inside a single transaction. Normal reads exclude soft-deleted items and
// subtasks, while the rows remain in the file with deleted_at set.
int softDeleteSurvivesRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0) {
        return 1;
    }

    const QString parentTitle = QStringLiteral("Plan the release");
    const QString subtaskTitle = QStringLiteral("Draft notes");
    const QString grandchildTitle = QStringLiteral("Verify changelog entries");
    qlonglong parentId = 0;
    qlonglong subtaskId = 0;
    qlonglong grandchildId = 0;

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

        Item grandchild;
        grandchild.kind = QStringLiteral("task");
        grandchild.title = grandchildTitle;
        grandchild.parentId = subtaskId;
        grandchild.sortOrder = 3;
        const std::optional<Item> createdGrandchild = items.create(grandchild);
        if (!createdGrandchild) {
            return fail(QStringLiteral("the grandchild subtask could not be created: %1")
                            .arg(items.lastError()));
        }
        grandchildId = createdGrandchild->id;

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

        if (!check(!items.readById(parentId)
                       && !items.readById(subtaskId)
                       && !items.readById(grandchildId),
                   QStringLiteral(
                       "a soft-deleted Item or descendant subtask was returned by a normal read"))) {
            return 1;
        }
        const QList<Item> tasks = items.readByKind(QStringLiteral("task"));
        if (!check(tasks.isEmpty(),
                   QStringLiteral(
                       "a soft-deleted Item or descendant subtask appeared in a normal query"))) {
            return 1;
        }
        if (!check(items.readByParent(parentId).isEmpty()
                       && items.readByParent(subtaskId).isEmpty(),
                   QStringLiteral("soft-delete did not cascade to all descendant subtasks"))) {
            return 1;
        }

        // A store-layer read observes the rows that normal reads hide: proof
        // they remained in the SQLite file across the restart. This is not a
        // Trash UI and no product surface is involved.
        const std::optional<Item> stored =
            items.readByIdIncludingSoftDeleted(parentId);
        const std::optional<Item> storedSubtask =
            items.readByIdIncludingSoftDeleted(subtaskId);
        const std::optional<Item> storedGrandchild =
            items.readByIdIncludingSoftDeleted(grandchildId);
        if (!check(stored.has_value() && storedSubtask.has_value()
                       && storedGrandchild.has_value(),
                   QStringLiteral(
                       "a soft-deleted row did not remain in the file"))) {
            return 1;
        }
        if (!check(stored->deletedAt.has_value()
                       && QDateTime::fromString(*stored->deletedAt,
                                                Qt::ISODate).isValid()
                       && storedSubtask->deletedAt.has_value()
                       && QDateTime::fromString(*storedSubtask->deletedAt,
                                                Qt::ISODate).isValid()
                       && storedGrandchild->deletedAt.has_value()
                       && QDateTime::fromString(*storedGrandchild->deletedAt,
                                                Qt::ISODate).isValid(),
                   QStringLiteral(
                       "deleted_at is not an ISO 8601 soft-delete marker on the entire subtree"))) {
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

int itemSelectionRenameCompletionAndSoftDeleteSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString firstId;
    QString secondId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!check(!itemModel.hasSelection() && itemModel.selectedId().isEmpty()
                       && itemModel.title().isEmpty() && !itemModel.completed()
                       && itemModel.error().isEmpty(),
                   QStringLiteral("ItemModel did not start in an idle empty state")))
            return 1;

        ItemRepository items(database);
        Item event;
        event.kind = QStringLiteral("event");
        event.title = QStringLiteral("Standup");
        event.startAt = todayModel.selectedDate() + QStringLiteral("T09:30:00");
        const auto createdEvent = items.create(event);
        if (!createdEvent)
            return fail(items.lastError());

        if (!todayModel.addTask(QStringLiteral("Draft release notes"))
            || !todayModel.addTask(QStringLiteral("Check build output")))
            return fail(QStringLiteral("could not seed Tasks for ItemModel test"));

        todayModel.refresh();
        const QVariantList initialLines = todayModel.lines();
        if (!check(initialLines.size() == 3 && todayModel.taskCount() == 2
                       && todayModel.eventCount() == 1,
                   QStringLiteral("seeded Tasks and Event missing from Today")))
            return 1;

        firstId = initialLines.at(0).toMap().value(QStringLiteral("id")).toString();
        secondId = initialLines.at(1).toMap().value(QStringLiteral("id")).toString();

        if (!check(!itemModel.selectTask(QString::number(createdEvent->id))
                       && !itemModel.error().isEmpty() && !itemModel.hasSelection(),
                   QStringLiteral("selecting a non-Task Item was not rejected")))
            return 1;

        if (!check(itemModel.selectTask(firstId) && itemModel.hasSelection()
                       && itemModel.selectedId() == firstId
                       && itemModel.title() == QStringLiteral("Draft release notes")
                       && !itemModel.completed() && itemModel.error().isEmpty(),
                   QStringLiteral("selecting first Task did not populate ItemModel")))
            return 1;

        itemModel.clearSelection();
        if (!check(!itemModel.hasSelection() && itemModel.selectedId().isEmpty()
                       && itemModel.title().isEmpty(),
                   QStringLiteral("clearSelection did not return ItemModel to idle")))
            return 1;

        if (!check(itemModel.selectTask(firstId),
                   QStringLiteral("could not re-select first Task")))
            return 1;

        if (!check(!itemModel.renameTask(firstId, QStringLiteral("   "))
                       && !itemModel.error().isEmpty()
                       && itemModel.title() == QStringLiteral("Draft release notes"),
                   QStringLiteral("blank inline rename was not rejected")))
            return 1;

        if (!check(itemModel.renameTask(firstId, QStringLiteral("  Publish release notes  "))
                       && itemModel.error().isEmpty()
                       && itemModel.title() == QStringLiteral("Publish release notes")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("title")).toString()
                           == QStringLiteral("Publish release notes"),
                   QStringLiteral("renaming selected Task did not sync to Today immediately")))
            return 1;

        if (!check(itemModel.renameTask(secondId, QStringLiteral("Verify local build"))
                       && itemModel.selectedId() == firstId
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("title")).toString()
                           == QStringLiteral("Verify local build"),
                   QStringLiteral("renaming non-selected Task did not sync to Today")))
            return 1;

        if (!check(itemModel.setTaskCompleted(firstId, true)
                       && itemModel.completed()
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("completed")).toBool(),
                   QStringLiteral("completing Task via ItemModel did not sync")))
            return 1;

        if (!check(itemModel.setTaskCompleted(firstId, false)
                       && !itemModel.completed()
                       && !todayModel.lines().at(0).toMap().value(QStringLiteral("completed")).toBool(),
                   QStringLiteral("reopening Task via ItemModel did not sync")))
            return 1;

        if (!check(todayModel.setTaskCompleted(firstId, true) && itemModel.completed(),
                   QStringLiteral("completing Task via TodayModel did not sync ItemModel")))
            return 1;

        if (!check(itemModel.setTaskCompleted(secondId, true)
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("completed")).toBool(),
                   QStringLiteral("completing second Task via ItemModel did not sync")))
            return 1;

        // Simulated database write failure must preserve state and report an error.
        QSqlQuery query(database.connection());
        if (!query.exec(QStringLiteral(
                "CREATE TRIGGER fail_task_mutation BEFORE UPDATE ON items "
                "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'write blocked'); END")))
            return fail(query.lastError().text());

        if (!check(!itemModel.renameTask(firstId, QStringLiteral("Should fail"))
                       && itemModel.error().contains(QStringLiteral("write blocked"))
                       && itemModel.title() == QStringLiteral("Publish release notes")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("title")).toString()
                           == QStringLiteral("Publish release notes"),
                   QStringLiteral("failed rename did not preserve Task state or report error")))
            return 1;

        if (!check(!itemModel.deleteTask(firstId)
                       && itemModel.error().contains(QStringLiteral("write blocked"))
                       && itemModel.hasSelection() && todayModel.taskCount() == 2,
                   QStringLiteral("failed soft-delete cleared selection or removed Task")))
            return 1;

        if (!query.exec(QStringLiteral("DROP TRIGGER fail_task_mutation")))
            return fail(query.lastError().text());

        if (!check(itemModel.deleteTask(firstId) && itemModel.error().isEmpty()
                       && !itemModel.hasSelection() && itemModel.selectedId().isEmpty()
                       && todayModel.taskCount() == 1
                       && todayModel.summary() == QStringLiteral("1 task and 1 event on this day")
                       && todayModel.lines().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString()
                           == secondId,
                   QStringLiteral("soft-delete did not dismiss detail pane and update Today immediately")))
            return 1;
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        ItemRepository items(database);

        if (!check(todayModel.taskCount() == 1 && todayModel.eventCount() == 1
                       && todayModel.lines().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString()
                           == secondId
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("title")).toString()
                           == QStringLiteral("Verify local build")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("completed")).toBool(),
                   QStringLiteral("surviving renamed/completed Task did not persist across restart")))
            return 1;

        if (!check(itemModel.selectTask(secondId)
                       && itemModel.title() == QStringLiteral("Verify local build")
                       && itemModel.completed(),
                   QStringLiteral("reopened ItemModel did not load surviving Task after restart")))
            return 1;

        const auto deletedRow = items.readByIdIncludingSoftDeleted(firstId.toLongLong());
        if (!check(!items.readById(firstId.toLongLong())
                       && !itemModel.selectTask(firstId)
                       && deletedRow && deletedRow->deletedAt.has_value(),
                   QStringLiteral("soft-deleted Task did not remain excluded with deleted_at set after restart")))
            return 1;
    }

    return 0;
}

int priorityLevelsSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString firstId;
    QString secondId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Prepare sprint notes"))
            || !todayModel.addTask(QStringLiteral("Review pull requests")))
            return fail(QStringLiteral("could not seed Tasks for priority test"));

        const QVariantList lines = todayModel.lines();
        if (!check(lines.size() == 2,
                   QStringLiteral("seeded Tasks missing from Today")))
            return 1;

        firstId = lines.at(0).toMap().value(QStringLiteral("id")).toString();
        secondId = lines.at(1).toMap().value(QStringLiteral("id")).toString();

        if (!check(itemModel.selectTask(firstId) && itemModel.priority() == 0,
                   QStringLiteral("new Task did not default to priority 0")))
            return 1;

        if (!check(!itemModel.setTaskPriority(firstId, -1)
                       && !itemModel.error().isEmpty()
                       && itemModel.priority() == 0,
                   QStringLiteral("negative priority was not rejected")))
            return 1;

        if (!check(!itemModel.setTaskPriority(firstId, 4)
                       && !itemModel.error().isEmpty()
                       && itemModel.priority() == 0,
                   QStringLiteral("out-of-range priority was not rejected")))
            return 1;

        ItemRepository items(database);
        const auto beforeUpdate = items.readById(firstId.toLongLong());
        if (!beforeUpdate)
            return fail(QStringLiteral("could not read Task before priority update"));

        if (!check(itemModel.setTaskPriority(firstId, 3)
                       && itemModel.error().isEmpty()
                       && itemModel.priority() == 3
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("priority")).toInt()
                           == 3,
                   QStringLiteral("setting high priority did not sync ItemModel and TodayModel")))
            return 1;

        const auto afterUpdate = items.readById(firstId.toLongLong());
        if (!check(afterUpdate && afterUpdate->priority == 3
                       && afterUpdate->modifiedAt >= beforeUpdate->modifiedAt,
                   QStringLiteral("high priority did not persist to SQLite with modified_at")))
            return 1;

        if (!check(itemModel.setTaskPriority(firstId, 1)
                       && itemModel.priority() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("priority")).toInt()
                           == 1,
                   QStringLiteral("changing priority to low did not sync")))
            return 1;

        if (!check(itemModel.setTaskPriority(firstId, 0)
                       && itemModel.priority() == 0
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("priority")).toInt()
                           == 0,
                   QStringLiteral("clearing priority back to None did not sync")))
            return 1;

        if (!check(itemModel.setTaskPriority(secondId, 2)
                       && itemModel.selectedId() == firstId
                       && itemModel.priority() == 0
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("priority")).toInt()
                           == 2,
                   QStringLiteral("setting medium priority on non-selected Task did not sync")))
            return 1;

        // Simulated write failure must preserve priority and report an error.
        QSqlQuery query(database.connection());
        if (!query.exec(QStringLiteral(
                "CREATE TRIGGER fail_priority_update BEFORE UPDATE ON items "
                "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'priority blocked'); END")))
            return fail(query.lastError().text());

        if (!check(!itemModel.setTaskPriority(firstId, 2)
                       && itemModel.error().contains(QStringLiteral("priority blocked"))
                       && itemModel.priority() == 0
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("priority")).toInt()
                           == 0,
                   QStringLiteral("failed priority update did not preserve state or report error")))
            return 1;

        if (!query.exec(QStringLiteral("DROP TRIGGER fail_priority_update")))
            return fail(query.lastError().text());
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);

        if (!check(todayModel.lines().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("priority")).toInt()
                           == 0
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("priority")).toInt()
                           == 2,
                   QStringLiteral("priority values did not survive restart on TodayModel")))
            return 1;

        if (!check(itemModel.selectTask(secondId) && itemModel.priority() == 2,
                   QStringLiteral("priority value did not survive restart in ItemModel")))
            return 1;
    }

    return 0;
}

int plannedDurationSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString firstId;
    QString secondId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Write integration test"))
            || !todayModel.addTask(QStringLiteral("Clean up worktree")))
            return fail(QStringLiteral("could not seed Tasks for duration test"));

        const QVariantList lines = todayModel.lines();
        if (!check(lines.size() == 2,
                   QStringLiteral("seeded Tasks missing from Today")))
            return 1;

        firstId = lines.at(0).toMap().value(QStringLiteral("id")).toString();
        secondId = lines.at(1).toMap().value(QStringLiteral("id")).toString();

        if (!check(itemModel.selectTask(firstId)
                       && !itemModel.hasDuration()
                       && itemModel.durationMin() == 0
                       && lines.at(0).toMap().value(QStringLiteral("durationMin")).toInt() == 0,
                   QStringLiteral("new Task did not start with unset duration")))
            return 1;

        if (!check(!itemModel.setTaskDuration(firstId, 0)
                       && !itemModel.error().isEmpty()
                       && !itemModel.hasDuration()
                       && itemModel.durationMin() == 0,
                   QStringLiteral("zero duration was not rejected")))
            return 1;

        if (!check(!itemModel.setTaskDuration(firstId, -15)
                       && !itemModel.error().isEmpty()
                       && !itemModel.hasDuration(),
                   QStringLiteral("negative duration was not rejected")))
            return 1;

        ItemRepository items(database);
        const auto beforeUpdate = items.readById(firstId.toLongLong());
        if (!beforeUpdate)
            return fail(QStringLiteral("could not read Task before duration update"));

        if (!check(itemModel.setTaskDuration(firstId, 45)
                       && itemModel.error().isEmpty()
                       && itemModel.hasDuration()
                       && itemModel.durationMin() == 45
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 45,
                   QStringLiteral("setting planned duration did not sync ItemModel and TodayModel")))
            return 1;

        const auto afterUpdate = items.readById(firstId.toLongLong());
        if (!check(afterUpdate && afterUpdate->durationMin == std::optional<int>(45)
                       && afterUpdate->modifiedAt >= beforeUpdate->modifiedAt,
                   QStringLiteral("planned duration did not persist to SQLite with modified_at")))
            return 1;

        if (!check(itemModel.setTaskDuration(firstId, 90)
                       && itemModel.durationMin() == 90
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 90,
                   QStringLiteral("editing planned duration did not sync")))
            return 1;

        if (!check(itemModel.clearTaskDuration(firstId)
                       && itemModel.error().isEmpty()
                       && !itemModel.hasDuration()
                       && itemModel.durationMin() == 0
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 0,
                   QStringLiteral("clearing planned duration did not sync")))
            return 1;

        const auto afterClear = items.readById(firstId.toLongLong());
        if (!check(afterClear && !afterClear->durationMin.has_value(),
                   QStringLiteral("clearing planned duration did not clear SQLite duration_min")))
            return 1;

        if (!check(itemModel.setTaskDuration(secondId, 30)
                       && itemModel.selectedId() == firstId
                       && !itemModel.hasDuration()
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 30,
                   QStringLiteral("setting duration on non-selected Task did not sync")))
            return 1;

        // Simulated write failure must preserve duration and report an error.
        QSqlQuery query(database.connection());
        if (!query.exec(QStringLiteral(
                "CREATE TRIGGER fail_duration_update BEFORE UPDATE ON items "
                "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'duration blocked'); END")))
            return fail(query.lastError().text());

        if (!check(!itemModel.setTaskDuration(firstId, 25)
                       && itemModel.error().contains(QStringLiteral("duration blocked"))
                       && !itemModel.hasDuration()
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 0,
                   QStringLiteral("failed duration update did not preserve state or report error")))
            return 1;

        if (!query.exec(QStringLiteral("DROP TRIGGER fail_duration_update")))
            return fail(query.lastError().text());
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        ItemRepository items(database);

        if (!check(todayModel.lines().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 0
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("durationMin")).toInt()
                           == 30,
                   QStringLiteral("planned duration values did not survive restart on TodayModel")))
            return 1;

        if (!check(itemModel.selectTask(secondId)
                       && itemModel.hasDuration()
                       && itemModel.durationMin() == 30,
                   QStringLiteral("planned duration did not survive restart in ItemModel")))
            return 1;

        const auto clearedRow = items.readById(firstId.toLongLong());
        if (!check(clearedRow && !clearedRow->durationMin.has_value(),
                   QStringLiteral("cleared duration_min did not remain null after restart")))
            return 1;
    }

    return 0;
}

int dueDateSchedulingSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    const QString today = QDate::currentDate().toString(Qt::ISODate);
    const QString tomorrow = QDate::currentDate().addDays(1).toString(Qt::ISODate);
    QString firstId;
    QString secondId;
    QString thirdId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Submit expense claim"))
            || !todayModel.addTask(QStringLiteral("Water studio plants"))
            || !todayModel.addTask(QStringLiteral("Buy printer paper")))
            return fail(QStringLiteral("could not seed Tasks for due date test"));

        const QVariantList initialLines = todayModel.lines();
        if (!check(initialLines.size() == 3 && todayModel.taskCount() == 3,
                   QStringLiteral("seeded Tasks missing from Today")))
            return 1;

        firstId = initialLines.at(0).toMap().value(QStringLiteral("id")).toString();
        secondId = initialLines.at(1).toMap().value(QStringLiteral("id")).toString();
        thirdId = initialLines.at(2).toMap().value(QStringLiteral("id")).toString();

        if (!check(itemModel.selectTask(firstId)
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == today
                       && !itemModel.hasDueTime()
                       && itemModel.dueTime().isEmpty()
                       && itemModel.duePrecision() == QStringLiteral("day"),
                   QStringLiteral("Today Task did not expose day-only due date")))
            return 1;

        // Invalid calendar day inputs must be rejected without modifying state.
        const QStringList invalidDates{
            QString(),
            QStringLiteral("2026-02-30"),
            QStringLiteral("2026-13-01"),
            QStringLiteral("not-a-date"),
            today + QStringLiteral("T10:00"),
        };
        for (const QString& badDate : invalidDates) {
            if (!check(!itemModel.setTaskDueDate(firstId, badDate)
                           && !itemModel.error().isEmpty()
                           && itemModel.dueDate() == today
                           && itemModel.duePrecision() == QStringLiteral("day"),
                       QStringLiteral("invalid due date '%1' was not rejected").arg(badDate)))
                return 1;
        }

        // Reschedule first Task from Today to Tomorrow; detail pane stays open
        // while Today immediately removes the Task and updates summary counts.
        if (!check(itemModel.setTaskDueDate(firstId, tomorrow)
                       && itemModel.error().isEmpty()
                       && itemModel.hasSelection()
                       && itemModel.selectedId() == firstId
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == tomorrow
                       && !itemModel.hasDueTime()
                       && itemModel.duePrecision() == QStringLiteral("day")
                       && todayModel.taskCount() == 2
                       && todayModel.lines().size() == 2
                       && todayModel.summary() == QStringLiteral("2 tasks and 0 events on this day"),
                   QStringLiteral("rescheduling Task off Today did not update ItemModel and TodayModel")))
            return 1;

        todayModel.selectDate(tomorrow);
        if (!check(todayModel.taskCount() == 1
                       && todayModel.lines().size() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == firstId,
                   QStringLiteral("rescheduled Task did not appear on target day")))
            return 1;
        todayModel.selectDate(today);

        // Clear due date on second Task (making it undated) while keeping detail pane open.
        if (!check(itemModel.selectTask(secondId)
                       && itemModel.clearTaskDueDate(secondId)
                       && itemModel.error().isEmpty()
                       && itemModel.hasSelection()
                       && itemModel.selectedId() == secondId
                       && !itemModel.hasDueDate()
                       && itemModel.dueDate().isEmpty()
                       && !itemModel.hasDueTime()
                       && itemModel.dueTime().isEmpty()
                       && itemModel.duePrecision().isEmpty()
                       && todayModel.taskCount() == 1
                       && todayModel.lines().size() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == thirdId,
                   QStringLiteral("clearing due date did not remove Task from Today or keep detail pane open")))
            return 1;

        ItemRepository items(database);
        const auto clearedRow = items.readById(secondId.toLongLong());
        if (!check(clearedRow && !clearedRow->dueAt.has_value()
                       && !clearedRow->duePrecision.has_value(),
                   QStringLiteral("clearing due date did not null out due_at and due_precision in SQLite")))
            return 1;

        // Schedule the undated Task back onto Today via ItemModel.
        if (!check(itemModel.setTaskDueDate(secondId, today)
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == today
                       && itemModel.duePrecision() == QStringLiteral("day")
                       && todayModel.taskCount() == 2
                       && todayModel.lines().size() == 2,
                   QStringLiteral("scheduling undated Task onto Today did not update TodayModel")))
            return 1;

        // Clear third Task so we verify undated state survives restart too.
        if (!check(itemModel.clearTaskDueDate(thirdId)
                       && todayModel.taskCount() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == secondId,
                   QStringLiteral("clearing third Task due date failed")))
            return 1;

        // Simulated SQLite write failure must preserve due date and report error.
        QSqlQuery query(database.connection());
        if (!query.exec(QStringLiteral(
                "CREATE TRIGGER fail_due_date_update BEFORE UPDATE ON items "
                "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'due date blocked'); END")))
            return fail(query.lastError().text());

        if (!check(!itemModel.setTaskDueDate(secondId, tomorrow)
                       && itemModel.error().contains(QStringLiteral("due date blocked"))
                       && itemModel.dueDate() == today
                       && todayModel.taskCount() == 1,
                   QStringLiteral("failed due date update did not preserve state or report error")))
            return 1;

        if (!query.exec(QStringLiteral("DROP TRIGGER fail_due_date_update")))
            return fail(query.lastError().text());
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        ItemRepository items(database);

        if (!check(todayModel.taskCount() == 1
                       && todayModel.lines().size() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == secondId,
                   QStringLiteral("Today due-date state did not survive restart")))
            return 1;

        todayModel.selectDate(tomorrow);
        if (!check(todayModel.taskCount() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == firstId,
                   QStringLiteral("Tomorrow due-date state did not survive restart")))
            return 1;

        if (!check(itemModel.selectTask(firstId)
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == tomorrow
                       && itemModel.duePrecision() == QStringLiteral("day"),
                   QStringLiteral("rescheduled Task did not reload due date in ItemModel after restart")))
            return 1;

        if (!check(itemModel.selectTask(thirdId)
                       && !itemModel.hasDueDate()
                       && itemModel.dueDate().isEmpty()
                       && itemModel.duePrecision().isEmpty(),
                   QStringLiteral("undated Task did not stay undated in ItemModel after restart")))
            return 1;

        const auto thirdRow = items.readById(thirdId.toLongLong());
        if (!check(thirdRow && !thirdRow->dueAt.has_value()
                       && !thirdRow->duePrecision.has_value(),
                   QStringLiteral("undated Task did not remain null in SQLite after restart")))
            return 1;
    }

    return 0;
}

int dueTimeSchedulingSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    const QString today = QDate::currentDate().toString(Qt::ISODate);
    const QString tomorrow = QDate::currentDate().addDays(1).toString(Qt::ISODate);
    QString timedId;
    QString dayOnlyId;
    QString undatedId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Call clinic"))
            || !todayModel.addTask(QStringLiteral("Mail signed contract"))
            || !todayModel.addTask(QStringLiteral("Sort archive box")))
            return fail(QStringLiteral("could not seed Tasks for due time test"));

        const QVariantList initialLines = todayModel.lines();
        if (!check(initialLines.size() == 3,
                   QStringLiteral("seeded Tasks missing from Today")))
            return 1;

        timedId = initialLines.at(0).toMap().value(QStringLiteral("id")).toString();
        dayOnlyId = initialLines.at(1).toMap().value(QStringLiteral("id")).toString();
        undatedId = initialLines.at(2).toMap().value(QStringLiteral("id")).toString();

        if (!check(itemModel.clearTaskDueDate(undatedId)
                       && itemModel.selectTask(timedId)
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("dueTime")).toString().isEmpty(),
                   QStringLiteral("day-only Task should not carry a dueTime")))
            return 1;

        // Adding a due time to an undated Task must be rejected.
        if (!check(!itemModel.setTaskDueTime(undatedId, QStringLiteral("14:30"))
                       && !itemModel.error().isEmpty(),
                   QStringLiteral("setting due time on undated Task was not rejected")))
            return 1;

        // Invalid 24-hour time inputs must be rejected.
        const QStringList invalidTimes{
            QString(),
            QStringLiteral("24:00"),
            QStringLiteral("12:60"),
            QStringLiteral("9:30"),
            QStringLiteral("14:30:00"),
            QStringLiteral("2:30pm"),
            QStringLiteral("abcd"),
        };
        for (const QString& badTime : invalidTimes) {
            if (!check(!itemModel.setTaskDueTime(timedId, badTime)
                           && !itemModel.error().isEmpty()
                           && !itemModel.hasDueTime()
                           && itemModel.duePrecision() == QStringLiteral("day"),
                       QStringLiteral("invalid due time '%1' was not rejected").arg(badTime)))
                return 1;
        }

        // Add 24-hour time to scheduled Task.
        if (!check(itemModel.setTaskDueTime(timedId, QStringLiteral("14:30"))
                       && itemModel.error().isEmpty()
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == today
                       && itemModel.hasDueTime()
                       && itemModel.dueTime() == QStringLiteral("14:30")
                       && itemModel.duePrecision() == QStringLiteral("time")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("dueTime")).toString()
                           == QStringLiteral("14:30")
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("dueTime")).toString().isEmpty(),
                   QStringLiteral("setting 24-hour due time did not sync ItemModel and TodayModel")))
            return 1;

        ItemRepository items(database);
        const auto timedRow = items.readById(timedId.toLongLong());
        if (!check(timedRow && timedRow->duePrecision == std::optional<QString>(QStringLiteral("time"))
                       && timedRow->dueAt
                       && QDateTime::fromString(*timedRow->dueAt, Qt::ISODate).toLocalTime().date().toString(Qt::ISODate) == today
                       && QDateTime::fromString(*timedRow->dueAt, Qt::ISODate).toLocalTime().time().toString(QStringLiteral("HH:mm"))
                           == QStringLiteral("14:30"),
                   QStringLiteral("timed due_at or due_precision did not persist accurately in SQLite")))
            return 1;

        // Edit 24-hour time.
        if (!check(itemModel.setTaskDueTime(timedId, QStringLiteral("08:05"))
                       && itemModel.dueTime() == QStringLiteral("08:05")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("dueTime")).toString()
                           == QStringLiteral("08:05"),
                   QStringLiteral("editing 24-hour due time did not sync")))
            return 1;

        // Update both date and time together, moving the Task to Tomorrow while keeping pane open.
        if (!check(itemModel.setTaskDueDateTime(timedId, tomorrow, QStringLiteral("19:45"))
                       && itemModel.hasSelection()
                       && itemModel.selectedId() == timedId
                       && itemModel.dueDate() == tomorrow
                       && itemModel.dueTime() == QStringLiteral("19:45")
                       && itemModel.duePrecision() == QStringLiteral("time")
                       && todayModel.taskCount() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == dayOnlyId,
                   QStringLiteral("rescheduling timed Task to Tomorrow did not update Today")))
            return 1;

        todayModel.selectDate(tomorrow);
        if (!check(todayModel.taskCount() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("dueTime")).toString()
                           == QStringLiteral("19:45"),
                   QStringLiteral("Tomorrow did not show rescheduled timed Task with 19:45 badge")))
            return 1;

        // Clear time back to day-only.
        if (!check(itemModel.clearTaskDueTime(timedId)
                       && itemModel.error().isEmpty()
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == tomorrow
                       && !itemModel.hasDueTime()
                       && itemModel.dueTime().isEmpty()
                       && itemModel.duePrecision() == QStringLiteral("day")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("dueTime")).toString().isEmpty(),
                   QStringLiteral("clearing due time did not revert Task to day-only")))
            return 1;

        const auto dayRevertedRow = items.readById(timedId.toLongLong());
        if (!check(dayRevertedRow && dayRevertedRow->dueAt == std::optional<QString>(tomorrow)
                       && dayRevertedRow->duePrecision == std::optional<QString>(QStringLiteral("day")),
                   QStringLiteral("clearing due time did not store day-only values in SQLite")))
            return 1;

        // Re-apply timed schedule for restart check on Today.
        if (!check(itemModel.setTaskDueDateTime(timedId, today, QStringLiteral("09:15")),
                   QStringLiteral("could not set timed schedule for restart verification")))
            return 1;

        // Simulated write failure preserves previous due time and reports error.
        QSqlQuery query(database.connection());
        if (!query.exec(QStringLiteral(
                "CREATE TRIGGER fail_due_time_update BEFORE UPDATE ON items "
                "WHEN NEW.kind = 'task' BEGIN SELECT RAISE(FAIL, 'due time blocked'); END")))
            return fail(query.lastError().text());

        if (!check(!itemModel.setTaskDueTime(timedId, QStringLiteral("22:00"))
                       && itemModel.error().contains(QStringLiteral("due time blocked"))
                       && itemModel.dueTime() == QStringLiteral("09:15")
                       && itemModel.duePrecision() == QStringLiteral("time"),
                   QStringLiteral("failed due time update did not preserve state or report error")))
            return 1;

        if (!query.exec(QStringLiteral("DROP TRIGGER fail_due_time_update")))
            return fail(query.lastError().text());
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);

        if (!check(todayModel.taskCount() == 2 && todayModel.lines().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString() == timedId
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("dueTime")).toString()
                           == QStringLiteral("09:15")
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("id")).toString() == dayOnlyId
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("dueTime")).toString().isEmpty(),
                   QStringLiteral("timed and day-only Tasks did not survive restart on TodayModel")))
            return 1;

        if (!check(itemModel.selectTask(timedId)
                       && itemModel.hasDueDate()
                       && itemModel.dueDate() == today
                       && itemModel.hasDueTime()
                       && itemModel.dueTime() == QStringLiteral("09:15")
                       && itemModel.duePrecision() == QStringLiteral("time"),
                   QStringLiteral("timed Task did not reload in ItemModel after restart")))
            return 1;
    }

    return 0;
}

int folderAndTagRepositoriesSurviveRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    qlonglong workFolderId = 0;
    qlonglong homeFolderId = 0;
    qlonglong focusTagId = 0;
    qlonglong healthTagId = 0;
    qlonglong taskId = 0;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        FolderRepository folders(database);
        TagRepository tags(database);
        ItemRepository items(database);

        if (!check(!folders.create(QStringLiteral("   ")) && !folders.lastError().isEmpty(),
                   QStringLiteral("blank Folder name was not rejected")))
            return 1;

        const auto work = folders.create(QStringLiteral("Work"));
        const auto home = folders.create(QStringLiteral("Personal"));
        if (!work || !home)
            return fail(folders.lastError());
        workFolderId = work->id;
        homeFolderId = home->id;

        if (!check(folders.readAll().size() == 2 && folders.readById(workFolderId)->name == QStringLiteral("Work"),
                   QStringLiteral("created Folders could not be listed or read")))
            return 1;

        if (!check(!tags.create(QStringLiteral("  ")) && !tags.lastError().isEmpty(),
                   QStringLiteral("blank Tag name was not rejected")))
            return 1;

        const auto focus = tags.create(QStringLiteral("Focus"), QStringLiteral("#3B82F6"));
        const auto health = tags.findOrCreate(QStringLiteral("Health"));
        if (!focus || !health)
            return fail(tags.lastError());
        focusTagId = focus->id;
        healthTagId = health->id;

        // Creating a duplicate Tag by unique name fails cleanly; findOrCreate returns existing.
        if (!check(!tags.create(QStringLiteral("Focus")) && !tags.lastError().isEmpty(),
                   QStringLiteral("duplicate Tag creation was not rejected")))
            return 1;

        const auto reused = tags.findOrCreate(QStringLiteral("Focus"));
        if (!check(reused && reused->id == focusTagId
                       && reused->colour == std::optional<QString>(QStringLiteral("#3B82F6")),
                   QStringLiteral("findOrCreate did not return existing Tag by unique name")))
            return 1;

        Item task;
        task.kind = QStringLiteral("task");
        task.title = QStringLiteral("Prepare quarterly plan");
        task.folderId = workFolderId;
        const auto createdTask = items.create(task);
        if (!createdTask)
            return fail(items.lastError());
        taskId = createdTask->id;

        if (!check(tags.attachToItem(taskId, focusTagId)
                       && tags.attachToItem(taskId, healthTagId)
                       && tags.attachToItem(taskId, focusTagId),
                   QStringLiteral("attaching Tags to an Item failed")))
            return 1;

        // Invalid FK references fail loudly rather than being silently ignored.
        if (!check(!tags.attachToItem(999999, focusTagId) && !tags.lastError().isEmpty(),
                   QStringLiteral("attaching Tag to unknown Item was not rejected")))
            return 1;

        const QList<Tag> attached = tags.readForItem(taskId);
        if (!check(attached.size() == 2,
                   QStringLiteral("reading Tags for Item did not return both attached Tags")))
            return 1;

        if (!check(tags.detachFromItem(taskId, healthTagId)
                       && tags.readForItem(taskId).size() == 1
                       && tags.readForItem(taskId).first().id == focusTagId,
                   QStringLiteral("detaching Tag from Item failed")))
            return 1;

        // Re-attach second Tag so both survive restart.
        if (!check(tags.attachToItem(taskId, healthTagId),
                   QStringLiteral("re-attaching second Tag failed")))
            return 1;
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        FolderRepository folders(database);
        TagRepository tags(database);
        ItemRepository items(database);

        const QList<Folder> folderList = folders.readAll();
        if (!check(folderList.size() == 2
                       && folderList.at(0).id == workFolderId
                       && folderList.at(1).id == homeFolderId,
                   QStringLiteral("Folders did not survive restart")))
            return 1;

        const QList<Tag> tagList = tags.readAll();
        if (!check(tagList.size() == 2
                       && tags.findByName(QStringLiteral("Focus"))->id == focusTagId
                       && tags.findByName(QStringLiteral("Health"))->id == healthTagId,
                   QStringLiteral("Tags did not survive restart")))
            return 1;

        const auto task = items.readById(taskId);
        const QList<Tag> itemTags = tags.readForItem(taskId);
        if (!check(task && task->folderId == std::optional<qlonglong>(workFolderId)
                       && itemTags.size() == 2,
                   QStringLiteral("Item Folder or attached Tags did not survive restart")))
            return 1;
    }

    return 0;
}

int foldersAndTagsSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString firstId;
    QString secondId;
    QString workFolderId;
    QString personalFolderId;
    QString focusTagId;
    QString urgentTagId;
    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Draft release notes"))
            || !todayModel.addTask(QStringLiteral("Schedule clinic visit")))
            return fail(QStringLiteral("could not seed Tasks for Folder/Tag test"));

        const QVariantList initialLines = todayModel.lines();
        if (!check(initialLines.size() == 2,
                   QStringLiteral("seeded Tasks missing from Today")))
            return 1;

        firstId = initialLines.at(0).toMap().value(QStringLiteral("id")).toString();
        secondId = initialLines.at(1).toMap().value(QStringLiteral("id")).toString();

        if (!check(itemModel.selectTask(firstId)
                       && !itemModel.hasFolder()
                       && itemModel.folderId().isEmpty()
                       && itemModel.folderName().isEmpty()
                       && itemModel.tags().isEmpty()
                       && itemModel.folders().isEmpty()
                       && itemModel.availableTags().isEmpty(),
                   QStringLiteral("initial Task should have no Folder or Tags")))
            return 1;

        // Invalid Folder/Tag operations fail cleanly with diagnostics.
        if (!check(itemModel.createFolder(QStringLiteral("  ")).isEmpty()
                       && !itemModel.error().isEmpty(),
                   QStringLiteral("blank Folder creation was not rejected")))
            return 1;
        if (!check(!itemModel.attachTagByName(firstId, QStringLiteral("   "))
                       && !itemModel.error().isEmpty(),
                   QStringLiteral("blank Tag attachment was not rejected")))
            return 1;

        // Create Folder and file first Task; create second Folder directly on second Task.
        workFolderId = itemModel.createFolder(QStringLiteral("Work"));
        if (!check(!workFolderId.isEmpty()
                       && itemModel.error().isEmpty()
                       && itemModel.folders().size() == 1
                       && itemModel.setTaskFolder(firstId, workFolderId)
                       && itemModel.hasFolder()
                       && itemModel.folderId() == workFolderId
                       && itemModel.folderName() == QStringLiteral("Work")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("folderId")).toString()
                           == workFolderId
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("folderName")).toString()
                           == QStringLiteral("Work"),
                   QStringLiteral("assigning Folder did not sync ItemModel and TodayModel")))
            return 1;

        if (!check(itemModel.createAndAssignFolder(secondId, QStringLiteral("Health"))
                       && itemModel.folders().size() == 2
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("folderName")).toString()
                           == QStringLiteral("Health"),
                   QStringLiteral("createAndAssignFolder on non-selected Task failed")))
            return 1;

        personalFolderId = itemModel.createFolder(QStringLiteral("Personal"));
        if (!check(!personalFolderId.isEmpty()
                       && itemModel.setTaskFolder(firstId, personalFolderId)
                       && itemModel.folderId() == personalFolderId
                       && itemModel.folderName() == QStringLiteral("Personal")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("folderName")).toString()
                           == QStringLiteral("Personal"),
                   QStringLiteral("changing assigned Folder did not sync")))
            return 1;

        // Clear Folder on second Task.
        if (!check(itemModel.clearTaskFolder(secondId)
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("folderId")).toString().isEmpty()
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("folderName")).toString().isEmpty(),
                   QStringLiteral("clearing Task Folder did not sync TodayModel")))
            return 1;

        // Attach multiple Tags (create on the fly and reuse existing).
        if (!check(itemModel.attachTagByName(firstId, QStringLiteral("Focus"))
                       && itemModel.attachTagByName(firstId, QStringLiteral("Urgent"))
                       && itemModel.availableTags().size() == 2
                       && itemModel.tags().size() == 2,
                   QStringLiteral("attaching multiple Tags on selected Task failed")))
            return 1;

        focusTagId = itemModel.tags().at(0).toMap().value(QStringLiteral("id")).toString();
        urgentTagId = itemModel.tags().at(1).toMap().value(QStringLiteral("id")).toString();
        if (itemModel.tags().at(0).toMap().value(QStringLiteral("name")).toString() != QStringLiteral("Focus")) {
            std::swap(focusTagId, urgentTagId);
        }

        // Reuse existing Tag on second Task without duplicating in availableTags.
        if (!check(itemModel.attachTagByName(secondId, QStringLiteral("Focus"))
                       && itemModel.availableTags().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("tags")).toList().size() == 2
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("tags")).toList().size() == 1,
                   QStringLiteral("TodayModel lines did not reflect attached Tags")))
            return 1;

        // Detach one Tag from first Task.
        if (!check(itemModel.detachTag(firstId, urgentTagId)
                       && itemModel.tags().size() == 1
                       && itemModel.tags().at(0).toMap().value(QStringLiteral("id")).toString() == focusTagId
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("tags")).toList().size() == 1,
                   QStringLiteral("detaching Tag did not sync ItemModel and TodayModel")))
            return 1;

        // Re-attach Urgent so multiple Tags survive restart on first Task.
        if (!check(itemModel.attachTag(firstId, urgentTagId)
                       && itemModel.tags().size() == 2,
                   QStringLiteral("attaching existing Tag by id failed")))
            return 1;
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);

        if (!check(todayModel.lines().size() == 2
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("folderId")).toString()
                           == personalFolderId
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("folderName")).toString()
                           == QStringLiteral("Personal")
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("tags")).toList().size() == 2
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("folderId")).toString().isEmpty()
                       && todayModel.lines().at(1).toMap().value(QStringLiteral("tags")).toList().size() == 1,
                   QStringLiteral("Folder and Tag state on TodayModel did not survive restart")))
            return 1;

        if (!check(itemModel.selectTask(firstId)
                       && itemModel.hasFolder()
                       && itemModel.folderId() == personalFolderId
                       && itemModel.folderName() == QStringLiteral("Personal")
                       && itemModel.tags().size() == 2
                       && itemModel.folders().size() == 3
                       && itemModel.availableTags().size() == 2,
                   QStringLiteral("Folder and Tag state did not reload in ItemModel after restart")))
            return 1;
    }

    return 0;
}

int subtasksAndCascadingSoftDeleteSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString parentId;
    QString childOneId;
    QString childTwoId;
    QString grandchildId;
    QString deletedSubtaskId;
    QString standaloneTaskId;
    QString standaloneChildId;

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Prepare release"))
            || !todayModel.addTask(QStringLiteral("Review docs")))
            return fail(QStringLiteral("could not seed Tasks for subtask test"));

        const QVariantList initialLines = todayModel.lines();
        if (!check(initialLines.size() == 2,
                   QStringLiteral("seeded Tasks missing from Today")))
            return 1;

        parentId = initialLines.at(0).toMap().value(QStringLiteral("id")).toString();
        standaloneTaskId = initialLines.at(1).toMap().value(QStringLiteral("id")).toString();

        // Invalid subtask creation fails cleanly with diagnostics.
        if (!check(itemModel.addSubtask(parentId, QStringLiteral("   ")).isEmpty()
                       && !itemModel.error().isEmpty(),
                   QStringLiteral("blank subtask creation was not rejected")))
            return 1;
        if (!check(itemModel.addSubtask(QStringLiteral("999999"), QStringLiteral("Orphan")).isEmpty()
                       && !itemModel.error().isEmpty(),
                   QStringLiteral("subtask creation on missing parent was not rejected")))
            return 1;

        // Select parent and create 3 levels of nested subtasks.
        if (!check(itemModel.selectTask(parentId)
                       && !itemModel.hasParent()
                       && itemModel.parentId().isEmpty()
                       && itemModel.subtaskTotal() == 0
                       && itemModel.subtaskDone() == 0
                       && itemModel.subtasks().isEmpty(),
                   QStringLiteral("parent Task initial subtask state was unexpected")))
            return 1;

        childOneId = itemModel.addSubtask(parentId, QStringLiteral("Build Linux binary"));
        childTwoId = itemModel.addSubtask(parentId, QStringLiteral("Write changelog"));
        deletedSubtaskId = itemModel.addSubtask(parentId, QStringLiteral("Temporary check"));
        if (!check(!childOneId.isEmpty() && !childTwoId.isEmpty() && !deletedSubtaskId.isEmpty()
                       && itemModel.error().isEmpty()
                       && itemModel.subtaskTotal() == 3
                       && itemModel.subtaskDone() == 0
                       && itemModel.subtasks().size() == 3,
                   QStringLiteral("creating child subtasks did not update selected parent")))
            return 1;

        // Add depth-2 and depth-3 descendants under childOne.
        grandchildId = itemModel.addSubtask(childOneId, QStringLiteral("Run qmake6"));
        const QString greatGrandchildId =
            itemModel.addSubtask(grandchildId, QStringLiteral("Archive artifact"));
        if (!check(!grandchildId.isEmpty() && !greatGrandchildId.isEmpty(),
                   QStringLiteral("creating multi-level nested subtasks failed")))
            return 1;

        // Complete childOne, rename childTwo, and schedule childTwo on Today:
        // Today must show childTwo nested under parentId without duplicating it
        // as an unrelated top-level root Task.
        if (!check(itemModel.setTaskCompleted(childOneId, true)
                       && itemModel.renameTask(childTwoId, QStringLiteral("Polish changelog"))
                       && itemModel.setTaskDueDate(childTwoId, todayModel.selectedDate()),
                   QStringLiteral("updating subtasks failed")))
            return 1;

        // Also delete one leaf subtask while parentId is selected: parent stays
        // open and reflects 1/2 progress immediately.
        if (!check(itemModel.deleteTask(deletedSubtaskId)
                       && itemModel.hasSelection()
                       && itemModel.selectedId() == parentId
                       && itemModel.subtaskTotal() == 2
                       && itemModel.subtaskDone() == 1
                       && itemModel.subtasks().size() == 2,
                   QStringLiteral("deleting a leaf subtask did not refresh parent cleanly")))
            return 1;

        // Today should have 2 top-level Tasks (Prepare release, Review docs),
        // not 3 (childTwo must not be duplicated at top level).
        const QVariantList linesAfterSubtasks = todayModel.lines();
        if (!check(todayModel.taskCount() == 2 && linesAfterSubtasks.size() == 2,
                   QStringLiteral("nested subtasks duplicated top-level Today lines")))
            return 1;

        const QVariantMap parentRow = linesAfterSubtasks.at(0).toMap();
        const QVariantList nestedChildren = parentRow.value(QStringLiteral("subtasks")).toList();
        const QVariantMap firstChildRow = nestedChildren.at(0).toMap();
        const QVariantList grandchildren = firstChildRow.value(QStringLiteral("subtasks")).toList();
        const QVariantList greatGrandchildren =
            grandchildren.at(0).toMap().value(QStringLiteral("subtasks")).toList();
        if (!check(parentRow.value(QStringLiteral("subtaskDone")).toInt() == 1
                       && parentRow.value(QStringLiteral("subtaskTotal")).toInt() == 2
                       && nestedChildren.size() == 2
                       && nestedChildren.at(1).toMap().value(QStringLiteral("title")).toString()
                           == QStringLiteral("Polish changelog")
                       && grandchildren.size() == 1
                       && grandchildren.at(0).toMap().value(QStringLiteral("id")).toString()
                           == grandchildId
                       && greatGrandchildren.size() == 1
                       && greatGrandchildren.at(0).toMap().value(QStringLiteral("id")).toString()
                           == greatGrandchildId,
                   QStringLiteral("TodayModel did not expose accurate nested subtask tree and counts")))
            return 1;

        // Navigate into a nested subtask in ItemModel and verify parent metadata.
        if (!check(itemModel.selectTask(grandchildId)
                       && itemModel.hasSelection()
                       && itemModel.selectedId() == grandchildId
                       && itemModel.hasParent()
                       && itemModel.parentId() == childOneId
                       && itemModel.parentTitle() == QStringLiteral("Build Linux binary")
                       && itemModel.subtaskTotal() == 1,
                   QStringLiteral("navigating into nested subtask did not populate parent and child state")))
            return 1;

        // Soft-delete ancestor parentId while grandchildId is open: detail pane
        // must close and the entire subtree must disappear from Today immediately.
        if (!check(itemModel.deleteTask(parentId)
                       && !itemModel.hasSelection()
                       && itemModel.selectedId().isEmpty()
                       && todayModel.taskCount() == 1
                       && todayModel.lines().size() == 1
                       && todayModel.lines().at(0).toMap().value(QStringLiteral("id")).toString()
                           == standaloneTaskId,
                   QStringLiteral("cascading ancestor soft-delete did not close descendant pane or clear Today")))
            return 1;

        // Add a surviving subtask tree on standaloneTaskId to verify restart survival.
        standaloneChildId = itemModel.addSubtask(standaloneTaskId, QStringLiteral("Verify examples"));
        const QString standaloneGrandchildId =
            itemModel.addSubtask(standaloneChildId, QStringLiteral("Check links"));
        if (!check(!standaloneChildId.isEmpty() && !standaloneGrandchildId.isEmpty()
                       && itemModel.setTaskCompleted(standaloneGrandchildId, true),
                   QStringLiteral("seeding surviving nested subtasks failed")))
            return 1;
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        ItemRepository items(database);

        // Deleted ancestor and all descendants remain excluded across restart.
        if (!check(!items.readById(parentId.toLongLong())
                       && !items.readById(childOneId.toLongLong())
                       && !items.readById(childTwoId.toLongLong())
                       && !items.readById(grandchildId.toLongLong())
                       && !items.readById(deletedSubtaskId.toLongLong()),
                   QStringLiteral("soft-deleted ancestor or descendant subtask remained active after restart")))
            return 1;

        // Surviving Task and its multi-depth subtasks survive restart intact.
        if (!check(todayModel.taskCount() == 1 && todayModel.lines().size() == 1,
                   QStringLiteral("Today task count changed across restart")))
            return 1;
        const QVariantMap survivingRow = todayModel.lines().at(0).toMap();
        const QVariantList survivingChildren = survivingRow.value(QStringLiteral("subtasks")).toList();
        if (!check(survivingRow.value(QStringLiteral("subtaskDone")).toInt() == 0
                       && survivingRow.value(QStringLiteral("subtaskTotal")).toInt() == 1
                       && survivingChildren.size() == 1
                       && survivingChildren.at(0).toMap().value(QStringLiteral("subtaskDone")).toInt() == 1
                       && survivingChildren.at(0).toMap().value(QStringLiteral("subtaskTotal")).toInt() == 1,
                   QStringLiteral("surviving nested subtask progress did not survive restart")))
            return 1;

        if (!check(itemModel.selectTask(standaloneChildId)
                       && itemModel.hasParent()
                       && itemModel.parentId() == standaloneTaskId
                       && itemModel.subtaskDone() == 1
                       && itemModel.subtaskTotal() == 1
                       && itemModel.subtasks().size() == 1,
                   QStringLiteral("ItemModel did not reload nested subtask state after restart")))
            return 1;
    }

    return 0;
}

int taskNoteScaffoldSyncAndPersistAcrossRestart()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString firstTaskId;
    QString secondTaskId;
    QString todayDayNoteText = QString::fromUtf8("Day note stays separate");
    const QString firstWriting = QString::fromUtf8("Context line\n\nسلام");
    const QString secondWriting = QString::fromUtf8("Second task note");

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        todayModel.setDayNoteText(todayDayNoteText);
        if (!todayModel.flushDayNote())
            return fail(todayModel.dayNoteError());

        if (!todayModel.addTask(QStringLiteral("Draft architecture"))
            || !todayModel.addTask(QStringLiteral("Prepare release notes")))
            return fail(QStringLiteral("could not seed Tasks for Task Note test"));

        const QVariantList lines = todayModel.lines();
        if (!check(lines.size() == 2, QStringLiteral("seeded Tasks missing from Today")))
            return 1;
        firstTaskId = lines.at(0).toMap().value(QStringLiteral("id")).toString();
        secondTaskId = lines.at(1).toMap().value(QStringLiteral("id")).toString();

        if (!check(!itemModel.property("noteEditable").toBool()
                       && itemModel.property("noteText").toString().isEmpty()
                       && itemModel.property("noteError").toString().isEmpty(),
                   QStringLiteral("unselected ItemModel should have non-editable empty Note")))
            return 1;

        if (!check(itemModel.selectTask(firstTaskId)
                       && itemModel.property("noteEditable").toBool()
                       && itemModel.property("noteText").toString().isEmpty()
                       && itemModel.property("noteError").toString().isEmpty(),
                   QStringLiteral("selecting a fresh Task did not expose editable empty Note")))
            return 1;

        BlockRepository blocks(database);
        itemModel.setNoteText(QStringLiteral("first\nsecond"));
        if (!check(itemModel.noteText() == QStringLiteral("first\nsecond")
                       && blocks.readByItem(firstTaskId.toLongLong()).isEmpty(),
                   QStringLiteral("Task Note saved before debounce or did not update property")))
            return 1;

        auto wait = [](int ms) {
            QEventLoop loop;
            QTimer::singleShot(ms, &loop, &QEventLoop::quit);
            loop.exec();
        };
        wait(250);
        itemModel.setNoteText(QStringLiteral("first\nchanged"));
        wait(250);
        if (!check(blocks.readByItem(firstTaskId.toLongLong()).isEmpty(),
                   QStringLiteral("Task Note debounce was not restarted on edit")))
            return 1;

        wait(300);
        QList<Block> stored = blocks.readByItem(firstTaskId.toLongLong());
        if (!check(stored.size() == 2
                       && stored.at(0).type == QStringLiteral("text")
                       && stored.at(0).content == QStringLiteral("first")
                       && stored.at(1).type == QStringLiteral("text")
                       && stored.at(1).content == QStringLiteral("changed"),
                   QStringLiteral("debounced Task Note save did not persist ordered Blocks")))
            return 1;

        // Reverting pending edits back to saved text cancels pending write.
        itemModel.setNoteText(QStringLiteral("temporary"));
        itemModel.setNoteText(QStringLiteral("first\nchanged"));
        wait(500);
        stored = blocks.readByItem(firstTaskId.toLongLong());
        if (!check(stored.size() == 2
                       && stored.at(1).content == QStringLiteral("changed"),
                   QStringLiteral("reverting pending Task Note text modified stored Blocks")))
            return 1;

        // Task metadata updates must not clobber pending unsaved Note text.
        itemModel.setNoteText(firstWriting);
        if (!check(itemModel.setTaskPriority(firstTaskId, 2)
                       && itemModel.noteText() == firstWriting,
                   QStringLiteral("updating Task metadata clobbered pending Task Note text")))
            return 1;

        // Switching Tasks flushes the outgoing Task's dirty Note and loads the next.
        if (!check(itemModel.selectTask(secondTaskId)
                       && itemModel.selectedId() == secondTaskId
                       && itemModel.noteEditable()
                       && itemModel.noteText().isEmpty(),
                   QStringLiteral("switching Task did not load second Task Note")))
            return 1;

        const QList<Block> firstAfterSwitch = blocks.readByItem(firstTaskId.toLongLong());
        if (!check(firstAfterSwitch.size() == 3
                       && firstAfterSwitch.at(0).content == QStringLiteral("Context line")
                       && firstAfterSwitch.at(1).content.isEmpty()
                       && firstAfterSwitch.at(2).content == QString::fromUtf8("سلام"),
                   QStringLiteral("switching Task did not flush first Task Note to SQLite")))
            return 1;

        // Closing selection flushes dirty Note and clears state, then reopening restores it.
        itemModel.setNoteText(secondWriting);
        itemModel.clearSelection();
        if (!check(!itemModel.hasSelection()
                       && !itemModel.noteEditable()
                       && itemModel.noteText().isEmpty()
                       && itemModel.noteError().isEmpty(),
                   QStringLiteral("closing selection did not reset Task Note state")))
            return 1;

        const QList<Block> secondAfterClose = blocks.readByItem(secondTaskId.toLongLong());
        if (!check(secondAfterClose.size() == 1
                       && secondAfterClose.at(0).content == secondWriting,
                   QStringLiteral("closing selection did not flush second Task Note")))
            return 1;

        if (!check(itemModel.selectTask(firstTaskId)
                       && itemModel.noteText() == firstWriting,
                   QStringLiteral("reopening Task did not restore its Note")))
            return 1;

        // Editing Task Note leaves Today's Day note unaffected, and vice versa.
        if (!check(todayModel.dayNoteText() == todayDayNoteText,
                   QStringLiteral("editing Task Note affected Today's Day note")))
            return 1;

        // Unflushed Task Note edit is flushed on ItemModel destruction before restart.
        itemModel.setNoteText(firstWriting + QStringLiteral("\nPersisted on close"));
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        BlockRepository blocks(database);

        if (!check(todayModel.dayNoteText() == todayDayNoteText,
                   QStringLiteral("Today's Day note changed after Task Note restart")))
            return 1;

        if (!check(itemModel.selectTask(firstTaskId)
                       && itemModel.noteEditable()
                       && itemModel.noteText() == firstWriting + QStringLiteral("\nPersisted on close")
                       && itemModel.noteError().isEmpty(),
                   QStringLiteral("first Task Note did not survive restart")))
            return 1;

        if (!check(itemModel.selectTask(secondTaskId)
                       && itemModel.noteEditable()
                       && itemModel.noteText() == secondWriting
                       && blocks.readByItem(secondTaskId.toLongLong()).size() == 1,
                   QStringLiteral("second Task Note did not survive restart")))
            return 1;
    }

    return 0;
}

int taskNotePlainTextGuardAndFailureSafety()
{
    QTemporaryDir dataDir;
    if (freshAppData(&dataDir) != 0)
        return 1;

    QString formattedTaskId;
    QString inlineJsonTaskId;
    QString plainTaskId;
    QString otherTaskId;
    QString deletedDirtyTaskId;
    qlonglong headingBlockId = 0;
    qlonglong inlineJsonBlockId = 0;

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        TodayModel todayModel(database);
        ItemModel itemModel(database);
        QObject::connect(&itemModel, &ItemModel::taskMutated,
                         &todayModel, &TodayModel::refresh);
        QObject::connect(&todayModel, &TodayModel::taskUpdated,
                         &itemModel, &ItemModel::syncTask);

        if (!todayModel.addTask(QStringLiteral("Formatted Task"))
            || !todayModel.addTask(QStringLiteral("Inline Formatted Task"))
            || !todayModel.addTask(QStringLiteral("Plain Task"))
            || !todayModel.addTask(QStringLiteral("Other Task"))
            || !todayModel.addTask(QStringLiteral("Delete Dirty Task")))
            return fail(QStringLiteral("could not seed Tasks for Task Note safety test"));

        const QVariantList lines = todayModel.lines();
        if (!check(lines.size() == 5, QStringLiteral("seeded Tasks missing")))
            return 1;

        formattedTaskId = lines.at(0).toMap().value(QStringLiteral("id")).toString();
        inlineJsonTaskId = lines.at(1).toMap().value(QStringLiteral("id")).toString();
        plainTaskId = lines.at(2).toMap().value(QStringLiteral("id")).toString();
        otherTaskId = lines.at(3).toMap().value(QStringLiteral("id")).toString();
        deletedDirtyTaskId = lines.at(4).toMap().value(QStringLiteral("id")).toString();

        BlockRepository blocks(database);
        Block heading;
        heading.itemId = formattedTaskId.toLongLong();
        heading.type = QStringLiteral("heading");
        heading.content = QStringLiteral("Project Spec");
        const auto storedHeading = blocks.create(heading);
        if (!storedHeading)
            return fail(blocks.lastError());
        headingBlockId = storedHeading->id;

        Block inlineText;
        inlineText.itemId = inlineJsonTaskId.toLongLong();
        inlineText.type = QStringLiteral("text");
        inlineText.content = QStringLiteral("Bold text");
        inlineText.inlineJson = QStringLiteral("[{\"start\":0,\"end\":4,\"type\":\"bold\"}]");
        const auto storedInline = blocks.create(inlineText);
        if (!storedInline)
            return fail(blocks.lastError());
        inlineJsonBlockId = storedInline->id;

        // Selecting formatted Task marks Note read-only with clear diagnostic.
        if (!check(itemModel.selectTask(formattedTaskId)
                       && !itemModel.noteEditable()
                       && itemModel.noteText() == QStringLiteral("Project Spec")
                       && !itemModel.noteError().isEmpty(),
                   QStringLiteral("Task with heading Block was not marked read-only")))
            return 1;

        const QString formattedError = itemModel.noteError();
        itemModel.setNoteText(QStringLiteral("Overwrite attempt"));
        if (!check(itemModel.noteText() == QStringLiteral("Project Spec")
                       && itemModel.flushNote()
                       && blocks.readById(headingBlockId)->type == QStringLiteral("heading")
                       && blocks.readById(headingBlockId)->content == QStringLiteral("Project Spec"),
                   QStringLiteral("read-only Task Note accepted edits or overwrote heading Block")))
            return 1;

        // Unrelated Task error and subsequent success preserve noteError.
        if (!check(!itemModel.renameTask(formattedTaskId, QStringLiteral("   "))
                       && !itemModel.error().isEmpty()
                       && itemModel.noteError() == formattedError,
                   QStringLiteral("Task rename error clobbered Task Note diagnostic")))
            return 1;
        if (!check(itemModel.renameTask(formattedTaskId, QStringLiteral("Formatted Task Renamed"))
                       && itemModel.error().isEmpty()
                       && itemModel.noteError() == formattedError,
                   QStringLiteral("Task rename success cleared Task Note read-only diagnostic")))
            return 1;

        // Text Block with inline_json metadata is also guarded as non-plain-text.
        if (!check(itemModel.selectTask(inlineJsonTaskId)
                       && !itemModel.noteEditable()
                       && itemModel.noteText() == QStringLiteral("Bold text")
                       && !itemModel.noteError().isEmpty(),
                   QStringLiteral("Task with inline_json Block was not marked read-only")))
            return 1;
        itemModel.setNoteText(QStringLiteral("Stripped"));
        if (!check(blocks.readById(inlineJsonBlockId)->inlineJson.has_value(),
                   QStringLiteral("inline_json Block metadata was stripped")))
            return 1;

        // Write failure during Task switch or detail-pane close blocks navigation
        // and preserves unsaved Task Note edits until retry succeeds.
        if (!check(itemModel.selectTask(plainTaskId)
                       && itemModel.noteEditable()
                       && itemModel.noteError().isEmpty(),
                   QStringLiteral("plain Task Note did not return to editable")))
            return 1;

        QSqlQuery query(database.connection());
        if (!query.exec(QStringLiteral(
                "CREATE TRIGGER reject_task_note BEFORE INSERT ON blocks "
                "BEGIN SELECT RAISE(FAIL, 'disk write failed'); END")))
            return fail(query.lastError().text());

        itemModel.setNoteText(QStringLiteral("Unsaved Task context"));
        if (!check(!itemModel.selectTask(otherTaskId)
                       && itemModel.selectedId() == plainTaskId
                       && itemModel.noteText() == QStringLiteral("Unsaved Task context")
                       && itemModel.noteError().contains(QStringLiteral("Cannot switch Task"))
                       && itemModel.noteError().contains(QStringLiteral("disk write failed")),
                   QStringLiteral("failed Task Note save did not block Task switch with explanation")))
            return 1;

        itemModel.clearSelection();
        if (!check(itemModel.hasSelection()
                       && itemModel.selectedId() == plainTaskId
                       && itemModel.noteText() == QStringLiteral("Unsaved Task context")
                       && itemModel.noteError().contains(QStringLiteral("Cannot close Task details"))
                       && itemModel.noteError().contains(QStringLiteral("disk write failed")),
                   QStringLiteral("failed Task Note save did not block close with explanation")))
            return 1;

        if (!query.exec(QStringLiteral("DROP TRIGGER reject_task_note")))
            return fail(query.lastError().text());

        if (!check(itemModel.selectTask(otherTaskId)
                       && itemModel.selectedId() == otherTaskId
                       && itemModel.noteError().isEmpty(),
                   QStringLiteral("retry switch after failure did not flush and open next Task")))
            return 1;

        const auto persistedPlain = blocks.readByItem(plainTaskId.toLongLong());
        if (!check(persistedPlain.size() == 1
                       && persistedPlain.at(0).content == QStringLiteral("Unsaved Task context"),
                   QStringLiteral("retried switch did not persist previously unsaved Task Note")))
            return 1;

        // Soft-deleting the selected Task while its Note has pending edits
        // succeeds and closes the detail pane cleanly.
        if (!check(itemModel.selectTask(deletedDirtyTaskId),
                   QStringLiteral("could not select Task for dirty delete check")))
            return 1;
        itemModel.setNoteText(QStringLiteral("Pending before delete"));
        if (!check(itemModel.deleteTask(deletedDirtyTaskId)
                       && !itemModel.hasSelection()
                       && !itemModel.noteEditable()
                       && itemModel.noteText().isEmpty(),
                   QStringLiteral("deleting selected Task with pending Note edit did not close cleanly")))
            return 1;
    }

    {
        Database database;
        if (!database.open())
            return fail(database.lastError());

        BlockRepository blocks(database);
        const auto headingAfter = blocks.readById(headingBlockId);
        const auto inlineAfter = blocks.readById(inlineJsonBlockId);
        const auto plainAfter = blocks.readByItem(plainTaskId.toLongLong());
        if (!check(headingAfter && headingAfter->type == QStringLiteral("heading")
                       && headingAfter->content == QStringLiteral("Project Spec")
                       && inlineAfter && inlineAfter->inlineJson.has_value()
                       && plainAfter.size() == 1
                       && plainAfter.at(0).content == QStringLiteral("Unsaved Task context"),
                   QStringLiteral("guarded formatted Blocks or plain Task Note changed after restart")))
            return 1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
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

    if (midnightRollover() != 0) {
        return 1;
    }
    if (todayClockChecks() != 0) {
        return 1;
    }
    if (debouncedDayNoteWrites() != 0) {
        return 1;
    }
    if (replaceTextWithNoncontiguousPositions() != 0) {
        return 1;
    }
    if (todayDataSurvivesRestartTogether() != 0) {
        return 1;
    }
    if (todayTaskWriteSurvivesRestart() != 0) {
        return 1;
    }
    if (todayQueriesOnlySelectedDay() != 0) {
        return 1;
    }
    if (todayCompletionSurvivesRestart() != 0) {
        return 1;
    }
    if (todayCompletionRefreshesAlreadyStoredState() != 0) {
        return 1;
    }
    if (todayRefreshRetainsStateOnReadFailure() != 0) {
        return 1;
    }
    if (richNoteEditIsSafe() != 0) {
        return 1;
    }
    if (dayNoteSaveFailureBlocksNavigationWithExplanation() != 0) {
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
    if (itemSelectionRenameCompletionAndSoftDeleteSurviveRestart() != 0) {
        return 1;
    }
    if (priorityLevelsSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (plannedDurationSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (dueDateSchedulingSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (dueTimeSchedulingSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (folderAndTagRepositoriesSurviveRestart() != 0) {
        return 1;
    }
    if (foldersAndTagsSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (subtasksAndCascadingSoftDeleteSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (taskNoteScaffoldSyncAndPersistAcrossRestart() != 0) {
        return 1;
    }
    if (taskNotePlainTextGuardAndFailureSafety() != 0) {
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
