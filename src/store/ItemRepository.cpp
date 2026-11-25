#include "ItemRepository.h"

#include "Database.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {

// ISO 8601 timestamps are stored as text in the v0.1 schema.
QString nowIsoUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

// Date-only values name a calendar day. Timestamps without an offset are
// local wall times; timestamps with an offset (including Z) name an instant.
// Invalid values never belong to a day.
bool fallsOnLocalDay(const std::optional<QString>& value, const QDate& day)
{
    if (!value || !day.isValid())
        return false;
    if (value->size() == 10) {
        const QDate date = QDate::fromString(*value, Qt::ISODate);
        return date.isValid() && date.toString(Qt::ISODate) == *value && date == day;
    }
    const QDateTime timestamp = QDateTime::fromString(*value, Qt::ISODate);
    return timestamp.isValid() && timestamp.toLocalTime().date() == day;
}

// Nullable-column helpers live in StoreHelpers.h, shared with
// BlockRepository, so a change to them is made once.
using store::bindOptional;
using store::readOptionalText;
using store::readOptionalInt;
using store::readOptionalId;

// One authoritative ordered list of the items columns. Every SELECT, INSERT
// and UPDATE column list is built from it; itemFromQuery() and
// bindItemFields() mirror its order, so the column shape lives in one place
// (Duplicated Code / Shotgun Surgery).
const QStringList kItemColumns = {
    QStringLiteral("id"),            QStringLiteral("kind"),
    QStringLiteral("title"),         QStringLiteral("created_at"),
    QStringLiteral("modified_at"),   QStringLiteral("completed_at"),
    QStringLiteral("deleted_at"),    QStringLiteral("due_at"),
    QStringLiteral("due_precision"), QStringLiteral("start_at"),
    QStringLiteral("end_at"),        QStringLiteral("all_day"),
    QStringLiteral("priority"),      QStringLiteral("duration_min"),
    QStringLiteral("parent_id"),     QStringLiteral("folder_id"),
    QStringLiteral("day"),           QStringLiteral("sort_order")
};

// Every SELECT selects this one list, optionally qualified for a JOIN.
QString itemColumns(const QString& qualifier = QString())
{
    QStringList columns;
    columns.reserve(kItemColumns.size());
    for (const QString& column : kItemColumns) {
        columns.append(qualifier + column);
    }
    return columns.join(QStringLiteral(", "));
}

// INSERT and UPDATE write every column except the auto-assigned row id, in
// the same order as the bound values below.
QString itemWritableColumns()
{
    return kItemColumns.mid(1).join(QStringLiteral(", "));
}

QString itemInsertPlaceholders()
{
    QStringList placeholders;
    for (int i = 1; i < kItemColumns.size(); ++i) {
        placeholders.append(QStringLiteral("?"));
    }
    return placeholders.join(QStringLiteral(", "));
}

QString itemUpdateAssignments()
{
    QStringList assignments;
    for (int i = 1; i < kItemColumns.size(); ++i) {
        assignments.append(kItemColumns.at(i) + QStringLiteral(" = ?"));
    }
    return assignments.join(QStringLiteral(", "));
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

// The one authoritative mapping from a stored row to an Item. The indices
// mirror kItemColumns, the single ordered list every SELECT is built from.
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

// Bind every writable Item column (all of kItemColumns except the id) in that
// order, so create() and update() share one binding sequence.
void bindItemFields(QSqlQuery& query, const Item& item)
{
    query.addBindValue(item.kind);
    // items.title is NOT NULL DEFAULT '', but every writable column is bound
    // explicitly, so the schema default never applies. A default-constructed
    // (null) QString would otherwise bind as SQL NULL and violate the NOT NULL
    // constraint. Supply a non-null empty string so create() and update() both
    // store an untitled Item as the schema's empty-string default.
    query.addBindValue(item.title.isNull() ? QStringLiteral("") : item.title);
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
}

} // namespace

ItemRepository::ItemRepository(Database& database)
    : StoreRepository(database)
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
    // Mirror the value actually stored: a null title is written as the empty
    // string (see bindItemFields), so the returned Item carries that same
    // non-null empty string rather than a null QString that would disagree
    // with a subsequent read-back.
    if (stored.title.isNull()) {
        stored.title = QStringLiteral("");
    }
    if (stored.createdAt.isEmpty()) {
        stored.createdAt = timestamp;
    }
    if (stored.modifiedAt.isEmpty()) {
        stored.modifiedAt = timestamp;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("INSERT INTO items (%1) VALUES (%2)")
                      .arg(itemWritableColumns(), itemInsertPlaceholders()));
    bindItemFields(query, stored);
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
    return selectRows<Item>(statement, bindings,
                            QStringLiteral("could not read Items"),
                            itemFromQuery);
}

std::optional<Item> ItemRepository::readById(qlonglong id)
{
    const QList<Item> items = select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE id = ? AND deleted_at IS NULL")
            .arg(itemColumns()),
        QVariantList{ QVariant(id) });
    if (items.isEmpty()) {
        return std::nullopt;
    }
    return items.first();
}

std::optional<Item> ItemRepository::readByIdIncludingSoftDeleted(qlonglong id)
{
    const QList<Item> items = select(
        QStringLiteral("SELECT %1 FROM items WHERE id = ?")
            .arg(itemColumns()),
        QVariantList{ QVariant(id) });
    if (items.isEmpty()) {
        return std::nullopt;
    }
    return items.first();
}

QList<Item> ItemRepository::readByKind(const QString& kind)
{
    return select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE kind = ? AND deleted_at IS NULL "
                       "ORDER BY sort_order, id")
            .arg(itemColumns()),
        QVariantList{ QVariant(kind) });
}

QList<Item> ItemRepository::readTasksDueOn(const QString& day)
{
    const QList<Item> candidates = select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE kind = 'task' AND deleted_at IS NULL "
                       "AND due_at IS NOT NULL ORDER BY sort_order, id")
            .arg(itemColumns()), {});
    const QDate selected = QDate::fromString(day, Qt::ISODate);
    QList<Item> matches;
    for (const Item& item : candidates)
        if (fallsOnLocalDay(item.dueAt, selected))
            matches.append(item);
    return matches;
}

QList<Item> ItemRepository::readEventsStartingOn(const QString& day)
{
    const QList<Item> candidates = select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE kind = 'event' AND deleted_at IS NULL "
                       "AND start_at IS NOT NULL ORDER BY sort_order, id")
            .arg(itemColumns()), {});
    const QDate selected = QDate::fromString(day, Qt::ISODate);
    QList<Item> matches;
    for (const Item& item : candidates)
        if (fallsOnLocalDay(item.startAt, selected))
            matches.append(item);
    return matches;
}

QList<Item> ItemRepository::readByDay(const QString& day)
{
    return select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE day = ? AND deleted_at IS NULL ORDER BY id")
            .arg(itemColumns()),
        QVariantList{ QVariant(day) });
}

QList<Item> ItemRepository::readByFolder(qlonglong folderId)
{
    return select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE folder_id = ? AND deleted_at IS NULL "
                       "ORDER BY sort_order, id")
            .arg(itemColumns()),
        QVariantList{ QVariant(folderId) });
}

QList<Item> ItemRepository::readByParent(qlonglong parentId)
{
    return select(
        QStringLiteral("SELECT %1 FROM items "
                       "WHERE parent_id = ? AND deleted_at IS NULL "
                       "ORDER BY sort_order, id")
            .arg(itemColumns()),
        QVariantList{ QVariant(parentId) });
}

QList<Item> ItemRepository::readByTag(qlonglong tagId)
{
    return select(
        QStringLiteral("SELECT %1 FROM items "
                       "JOIN item_tags ON item_tags.item_id = items.id "
                       "WHERE item_tags.tag_id = ? AND items.deleted_at IS NULL "
                       "ORDER BY items.sort_order, items.id")
            .arg(itemColumns(QStringLiteral("items."))),
        QVariantList{ QVariant(tagId) });
}

bool ItemRepository::update(const Item& item)
{
    m_error.clear();

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("UPDATE items SET %1 WHERE id = ?")
                      .arg(itemUpdateAssignments()));
    bindItemFields(query, item);
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

    // Setting deleted_at is the only change needed to retire an Item. Every
    // normal read filters on deleted_at IS NULL, and the one_day_note_per_day
    // partial index uses the same predicate, so this UPDATE also drops a Day
    // note out of that index. The row keeps its day (visible through
    // readByIdIncludingSoftDeleted) but no longer reserves the date, leaving it
    // free for a replacement Day note.
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
