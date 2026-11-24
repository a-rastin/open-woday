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

} // namespace

ItemRepository::ItemRepository(Database& database)
    : m_database(database)
{
}

std::optional<Item> ItemRepository::create(const QString& kind,
                                          const QString& title)
{
    m_error.clear();
    const QString timestamp = nowIsoUtc();

    // Bound values only: no Item text is ever concatenated into SQL.
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO items (kind, title, created_at, modified_at) "
        "VALUES (?, ?, ?, ?)"));
    query.addBindValue(kind);
    query.addBindValue(title);
    query.addBindValue(timestamp);
    query.addBindValue(timestamp);
    if (!query.exec()) {
        fail(QStringLiteral("could not create the Item: %1")
                 .arg(query.lastError().text()));
        return std::nullopt;
    }

    const QVariant insertedId = query.lastInsertId();
    if (!insertedId.isValid()) {
        fail(QStringLiteral("the created Item has no id"));
        return std::nullopt;
    }

    Item item;
    item.id = insertedId.toLongLong();
    item.kind = kind;
    item.title = title;
    item.createdAt = timestamp;
    item.modifiedAt = timestamp;
    return item;
}

std::optional<Item> ItemRepository::readById(qlonglong id)
{
    m_error.clear();

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "SELECT id, kind, title, created_at, modified_at "
        "FROM items WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        fail(QStringLiteral("could not read Item %1: %2")
                 .arg(id).arg(query.lastError().text()));
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }

    Item item;
    item.id = query.value(0).toLongLong();
    item.kind = query.value(1).toString();
    item.title = query.value(2).toString();
    item.createdAt = query.value(3).toString();
    item.modifiedAt = query.value(4).toString();
    return item;
}

QString ItemRepository::lastError() const
{
    return m_error;
}

void ItemRepository::fail(const QString& reason)
{
    m_error = reason;
}
