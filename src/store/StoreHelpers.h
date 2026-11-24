#ifndef OPENWODAY_STORE_STOREHELPERS_H
#define OPENWODAY_STORE_STOREHELPERS_H

#include "Database.h"

#include <QList>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <optional>

// One shared home for the store layer's query plumbing. ItemRepository and
// BlockRepository used to carry identical copies of the nullable-column
// helpers, the bound-value SELECT runner and the last-error slot, so every
// change to them had to be made twice (Duplicated Code / Shotgun Surgery).
namespace store {

// A NULL column is represented by std::nullopt, so an absent optional value is
// never confused with an empty string or a zero. Bound values only: no Item or
// Block text is ever concatenated into SQL.
template <typename T>
QVariant bindOptional(const std::optional<T>& value)
{
    return value ? QVariant(*value) : QVariant();
}

inline std::optional<QString> readOptionalText(const QSqlQuery& query, int field)
{
    const QVariant value = query.value(field);
    if (value.isNull()) {
        return std::nullopt;
    }
    return value.toString();
}

inline std::optional<int> readOptionalInt(const QSqlQuery& query, int field)
{
    const QVariant value = query.value(field);
    if (value.isNull()) {
        return std::nullopt;
    }
    return value.toInt();
}

inline std::optional<qlonglong> readOptionalId(const QSqlQuery& query, int field)
{
    const QVariant value = query.value(field);
    if (value.isNull()) {
        return std::nullopt;
    }
    return value.toLongLong();
}

} // namespace store

// Common base for the store repositories: the borrowed connection, the last
// diagnostic and the single bound-value SELECT runner. Derived classes supply
// only their own row mapper and CRUD statements.
class StoreRepository
{
public:
    explicit StoreRepository(Database& database) : m_database(database) {}

    // Clear, human-readable diagnostic for a failed operation.
    QString lastError() const { return m_error; }

protected:
    // Execute one fixed SELECT statement with bound values and map every row
    // through mapper(). No SQL is assembled from Item or Block text.
    template <typename T, typename Mapper>
    QList<T> selectRows(const QString& statement, const QVariantList& bindings,
                        const QString& failurePrefix, Mapper mapper)
    {
        m_error.clear();

        QSqlQuery query(m_database.connection());
        query.prepare(statement);
        for (const QVariant& value : bindings) {
            query.addBindValue(value);
        }
        if (!query.exec()) {
            fail(QStringLiteral("%1: %2")
                     .arg(failurePrefix, query.lastError().text()));
            return {};
        }

        QList<T> rows;
        while (query.next()) {
            rows.append(mapper(query));
        }
        return rows;
    }

    void fail(const QString& reason) { m_error = reason; }

    Database& m_database;
    QString m_error;
};

#endif // OPENWODAY_STORE_STOREHELPERS_H
