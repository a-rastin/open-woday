#ifndef OPENWODAY_STORE_ITEMREPOSITORY_H
#define OPENWODAY_STORE_ITEMREPOSITORY_H

#include "StoreHelpers.h"

#include <QList>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <optional>

// One Item as it is stored. A Task, Note or Event is one Item of a particular
// kind (GLOSSARY.md). Every column of the items table is represented here, so
// a contributor works with any Item through one consistent persistence API.
struct Item
{
    qlonglong id = 0;
    QString kind;                        // 'task', 'note' or 'event'
    QString title;
    QString createdAt;                   // ISO 8601
    QString modifiedAt;                  // ISO 8601
    std::optional<QString> completedAt;  // ISO 8601
    std::optional<QString> deletedAt;    // ISO 8601; soft-delete marker
    std::optional<QString> dueAt;        // ISO 8601
    std::optional<QString> duePrecision; // 'day','time','week','month','someday'
    std::optional<QString> startAt;      // ISO 8601
    std::optional<QString> endAt;        // ISO 8601
    bool allDay = false;
    int priority = 0;                    // 0 none, 1 low, 2 medium, 3 high
    std::optional<int> durationMin;
    std::optional<qlonglong> parentId;   // subtasks
    std::optional<qlonglong> folderId;
    std::optional<QString> day;          // 'YYYY-MM-DD', a Day note's date
    int sortOrder = 0;
};

// ItemRepository is the store-layer API for Items. It writes straight to the
// single local SQLite file owned by Database; there is no in-memory cache, so
// a written Item survives a restart of the connection.
class ItemRepository : public StoreRepository
{
public:
    explicit ItemRepository(Database& database);

    // Create one Item of the given kind and title with the other columns at
    // their defaults. The thinnest create path, kept as a convenience.
    std::optional<Item> create(const QString& kind, const QString& title);

    // Create an Item from all of its fields. The id is assigned, and empty
    // timestamps are filled with the current UTC time. Returns std::nullopt
    // and sets lastError() on failure — including a clear diagnostic when the
    // one-Day-note-per-date rule rejects a duplicate day.
    std::optional<Item> create(const Item& item);

    // Normal reads never return soft-deleted Items.
    std::optional<Item> readById(qlonglong id);

    // A store-layer read used to observe that a soft-deleted row is still in
    // the file. It is not a Trash UI.
    std::optional<Item> readByIdIncludingSoftDeleted(qlonglong id);

    // Reads by id, kind, day, Folder, parent (subtasks) and Tag — the query
    // shapes later phases need — while staying plain CRUD: no Inbox grouping,
    // Today page or Task detail pane yet.
    QList<Item> readByKind(const QString& kind);
    // Match the stored date prefix for day-only values and timestamps alike.
    QList<Item> readTasksDueOn(const QString& day);
    QList<Item> readEventsStartingOn(const QString& day);
    QList<Item> readByDay(const QString& day);
    QList<Item> readByFolder(qlonglong folderId);
    QList<Item> readByParent(qlonglong parentId);
    QList<Item> readByTag(qlonglong tagId);

    // Write every stored column of an existing Item, identified by its id.
    // Returns false and sets lastError() on failure.
    bool update(const Item& item);

    // Soft-delete via deleted_at. There is no hard delete and no Trash UI, and
    // subtasks are not cascaded in this phase (that rule is behavioural and
    // belongs to a later phase).
    bool softDelete(qlonglong id);

private:
    // Execute one fixed SELECT statement with bound values and map every row
    // through itemFromQuery(). No SQL is assembled from Item text.
    QList<Item> select(const QString& statement,
                       const QVariantList& bindings);
};

#endif // OPENWODAY_STORE_ITEMREPOSITORY_H
