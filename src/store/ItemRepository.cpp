#include "ItemRepository.h"

#include "Database.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

// ISO 8601 timestamps are stored as text in the v0.1 schema.
QString nowIsoUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

// A NULL column is represented by std::nullopt, so an absent optional value is
// never confused with an empty string or a zero. Bound values only: no Item
// text is ever concatenated into SQL.
template <typename T>
QVariant bindOptional(const std::optional<T>& value)
{
    if (!value) {
        return QVariant();
    }
    return QVariant(*value);
}

std::optional<QString> readOptionalText(const QSqlQuery& query, int field)
{
    const QVariant value = query.value(field);
    if (value.isNull()) {
        return std::nullopt;
    }
    return value.toString();
}

std::optional<int> readOptionalInt(const QSqlQuery& query, int field)
{
    const QVariant value = query.value(field);
    if (value.isNull()) {
        return std::nullopt;
    }
    return value.toInt();
}

std::optional<qlonglong> readOptionalId(const QSqlQuery& query, int field)
{
    const QVariant value = query.value(field);
    if (value.isNull()) {
        return std::nullopt;
    }
    return value.toLongLong();
}

// The only partial unique index on items is the one-Day-note-per-date rule.
// Surface that rule with a clear diagnostic instead of a raw driver message;
// other constraint failures keep their driver's explanation.
QString writeFailureMessage(const QSqlQuery& query, const QString& operation,
                            const std::optional<QString>& day)
{
    const QString dbError = query.lastError().text();
    if (dbError.contains(QStringLiteral("UNIQUE constraint failed"))) {
        return QStringLiteral(
            "only one Day note per date is allowed; a Day note for day '%1' "
            "already exists: %2")
            .arg(day.value_or(QString()), dbError);
    }
    return QStringLiteral("%1: %2").arg(operation, dbError);
}

// The one authoritative mapping from a stored row to an Item. Every column of
// the items table has a place here, in the order of the SELECT statements.
Item itemFromQuery(const QSqlQuery& query)
{
    Item item;
    item.id = query.value(0).toLongLong();
    item.kind = query.value(1).toString();
    item.title = query.value(2).toString();
    item.createdAt = query.value(3).toString();
    item.modifiedAt = query.value(4).toString();
    item.completedAt = readOptionalText(query, 5);
    item.deletedAt = readOptionalText(query, 6);
    item.dueAt = readOptionalText(query, 7);
    item.duePrecision = readOptionalText(query, 8);
    item.startAt = readOptionalText(query, 9);
    item.endAt = readOptionalText(query, 10);
    item.allDay = query.value(11).toInt() != 0;
    item.priority = query.value(12).toInt();
    item.durationMin = readOptionalInt(query, 13);
    item.parentId = readOptionalId(query, 14);
    item.folderId = readOptionalId(query, 15);
    item.day = readOptionalText(query, 16);
    item.sortOrder = query.value(17).toInt();
    return item;
}

} // namespace

ItemRepository::ItemRepository(Database& database)
    : m_database(database)
{
}

std::optional<Item> ItemRepository::create(const QString& kind,
                                           const QString& title)
{
    Item item;
    item.kind = kind;
    item.title = title;
    return create(item);
}

std::optional<Item> ItemRepository::create(const Item& item)
{
    m_error.clear();
    const QString timestamp = nowIsoUtc();

    Item stored = item;
    if (stored.createdAt.isEmpty()) {
        stored.createdAt = timestamp;
    }
    if (stored.modifiedAt.isEmpty()) {
        stored.modifiedAt = timestamp;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO items (kind, title, created_at, modified_at, "
        "completed_at, deleted_at, due_at, due_precision, start_at, end_at, "
        "all_day, priority, duration_min, parent_id, folder_id, day, sort_order) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(stored.kind);
    query.addBindValue(stored.title);
    query.addBindValue(stored.createdAt);
    query.addBindValue(stored.modifiedAt);
    query.addBindValue(bindOptional(stored.completedAt));
    query.addBindValue(bindOptional(stored.deletedAt));
    query.addBindValue(bindOptional(stored.dueAt));
    query.addBindValue(bindOptional(stored.duePrecision));
    query.addBindValue(bindOptional(stored.startAt));
    query.addBindValue(bindOptional(stored.endAt));
    query.addBindValue(stored.allDay ? 1 : 0);
    query.addBindValue(stored.priority);
    query.addBindValue(bindOptional(stored.durationMin));
    query.addBindValue(bindOptional(stored.parentId));
    query.addBindValue(bindOptional(stored.folderId));
    query.addBindValue(bindOptional(stored.day));
    query.addBindValue(stored.sortOrder);
    if (!query.exec()) {
        fail(writeFailureMessage(query,
                                 QStringLiteral("could not create the Item"),
                                 stored.day));
        return std::nullopt;
    }

    const QVariant insertedId = query.lastInsertId();
    if (!insertedId.isValid()) {
        fail(QStringLiteral("the created Item has no id"));
        return std::nullopt;
    }

    stored.id = insertedId.toLongLong();
    return stored;
}

QList<Item> ItemRepository::select(const QString& statement,
                                   const QVariantList& bindings)
{
    m_error.clear();

    QSqlQuery query(m_database.connection());
    query.prepare(statement);
    for (const QVariant& value : bindings) {
        query.addBindValue(value);
    }
    if (!query.exec()) {
        fail(QStringLiteral("could not read Items: %1")
                 .arg(query.lastError().text()));
        return {};
    }

    QList<Item> items;
    while (query.next()) {
        items.append(itemFromQuery(query));
    }
    return items;
}

std::optional<Item> ItemRepository::readById(qlonglong id)
{
    const QList<Item> items = select(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at, completed_at, "
        "deleted_at, due_at, due_precision, start_at, end_at, all_day, "
        "priority, duration_min, parent_id, folder_id, day, sort_order "
        "FROM items WHERE id = ? AND deleted_at IS NULL"),
        QVariantList{ QVariant(id) });
    if (items.isEmpty()) {
        return std::nullopt;
    }
    return items.first();
}

std::optional<Item> ItemRepository::readByIdIncludingSoftDeleted(qlonglong id)
{
    const QList<Item> items = select(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at, completed_at, "
        "deleted_at, due_at, due_precision, start_at, end_at, all_day, "
        "priority, duration_min, parent_id, folder_id, day, sort_order "
        "FROM items WHERE id = ?"),
        QVariantList{ QVariant(id) });
    if (items.isEmpty()) {
        return std::nullopt;
    }
    return items.first();
}

QList<Item> ItemRepository::readByKind(const QString& kind)
{
    return select(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at, completed_at, "
        "deleted_at, due_at, due_precision, start_at, end_at, all_day, "
        "priority, duration_min, parent_id, folder_id, day, sort_order "
        "FROM items WHERE kind = ? AND deleted_at IS NULL "
        "ORDER BY sort_order, id"),
        QVariantList{ QVariant(kind) });
}

QList<Item> ItemRepository::readByDay(const QString& day)
{
    return select(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at, completed_at, "
        "deleted_at, due_at, due_precision, start_at, end_at, all_day, "
        "priority, duration_min, parent_id, folder_id, day, sort_order "
        "FROM items WHERE day = ? AND deleted_at IS NULL ORDER BY id"),
        QVariantList{ QVariant(day) });
}

QList<Item> ItemRepository::readByFolder(qlonglong folderId)
{
    return select(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at, completed_at, "
        "deleted_at, due_at, due_precision, start_at, end_at, all_day, "
        "priority, duration_min, parent_id, folder_id, day, sort_order "
        "FROM items WHERE folder_id = ? AND deleted_at IS NULL "
        "ORDER BY sort_order, id"),
        QVariantList{ QVariant(folderId) });
}

QList<Item> ItemRepository::readByParent(qlonglong parentId)
{
    return select(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at, completed_at, "
        "deleted_at, due_at, due_precision, start_at, end_at, all_day, "
        "priority, duration_min, parent_id, folder_id, day, sort_order "
        "FROM items WHERE parent_id = ? AND deleted_at IS NULL "
        "ORDER BY sort_order, id"),
        QVariantList{ QVariant(parentId) });
}

QList<Item> ItemRepository::readByTag(qlonglong tagId)
{
    return select(QStringLiteral(
        "SELECT items.id, items.kind, items.title, items.created_at, "
        "items.modified_at, items.completed_at, items.deleted_at, "
        "items.due_at, items.due_precision, items.start_at, items.end_at, "
        "items.all_day, items.priority, items.duration_min, items.parent_id, "
        "items.folder_id, items.day, items.sort_order "
        "FROM items JOIN item_tags ON item_tags.item_id = items.id "
        "WHERE item_tags.tag_id = ? AND items.deleted_at IS NULL "
        "ORDER BY items.sort_order, items.id"),
        QVariantList{ QVariant(tagId) });
}

bool ItemRepository::update(const Item& item)
{
    m_error.clear();

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "UPDATE items SET kind = ?, title = ?, created_at = ?, modified_at = ?, "
        "completed_at = ?, deleted_at = ?, due_at = ?, due_precision = ?, "
        "start_at = ?, end_at = ?, all_day = ?, priority = ?, duration_min = ?, "
        "parent_id = ?, folder_id = ?, day = ?, sort_order = ? WHERE id = ?"));
    query.addBindValue(item.kind);
    query.addBindValue(item.title);
    query.addBindValue(item.createdAt);
    query.addBindValue(item.modifiedAt);
    query.addBindValue(bindOptional(item.completedAt));
    query.addBindValue(bindOptional(item.deletedAt));
    query.addBindValue(bindOptional(item.dueAt));
    query.addBindValue(bindOptional(item.duePrecision));
    query.addBindValue(bindOptional(item.startAt));
    query.addBindValue(bindOptional(item.endAt));
    query.addBindValue(item.allDay ? 1 : 0);
    query.addBindValue(item.priority);
    query.addBindValue(bindOptional(item.durationMin));
    query.addBindValue(bindOptional(item.parentId));
    query.addBindValue(bindOptional(item.folderId));
    query.addBindValue(bindOptional(item.day));
    query.addBindValue(item.sortOrder);
    query.addBindValue(item.id);
    if (!query.exec()) {
        fail(writeFailureMessage(
            query, QStringLiteral("could not update Item %1").arg(item.id),
            item.day));
        return false;
    }
    if (query.numRowsAffected() != 1) {
        fail(QStringLiteral("no Item with id %1 could be updated")
                 .arg(item.id));
        return false;
    }
    return true;
}

bool ItemRepository::softDelete(qlonglong id)
{
    m_error.clear();

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "UPDATE items SET deleted_at = ? WHERE id = ? AND deleted_at IS NULL"));
    query.addBindValue(nowIsoUtc());
    query.addBindValue(id);
    if (!query.exec()) {
        fail(QStringLiteral("could not soft-delete Item %1: %2")
                 .arg(id).arg(query.lastError().text()));
        return false;
    }
    if (query.numRowsAffected() != 1) {
        fail(QStringLiteral("Item %1 is absent or already soft-deleted")
                 .arg(id));
        return false;
    }
    return true;
}

QString ItemRepository::lastError() const
{
    return m_error;
}

void ItemRepository::fail(const QString& reason)
{
    m_error = reason;
}
