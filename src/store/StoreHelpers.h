#ifndef OPENWODAY_STORE_STOREHELPERS_H
#define OPENWODAY_STORE_STOREHELPERS_H

#include "Database.h"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTime>
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

struct LocalTimestamp
{
    QDate date;
    std::optional<QTime> time;
};

inline std::optional<QDate> parseIsoCalendarDay(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.size() != 10) {
        return std::nullopt;
    }
    const QDate date = QDate::fromString(trimmed, Qt::ISODate);
    if (!date.isValid() || date.toString(Qt::ISODate) != trimmed) {
        return std::nullopt;
    }
    return date;
}

inline std::optional<QTime> parseIso24HourTime(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.size() != 5 || trimmed.at(2) != QLatin1Char(':')) {
        return std::nullopt;
    }
    const QTime time = QTime::fromString(trimmed, QStringLiteral("HH:mm"));
    if (!time.isValid() || time.toString(QStringLiteral("HH:mm")) != trimmed) {
        return std::nullopt;
    }
    return time;
}

// Date-only values name a calendar day. Timestamps without an offset are
// local wall times (parsed directly so daylight-saving gaps never shift);
// timestamps with an offset (including Z) name an instant converted to local
// time. Invalid values return std::nullopt.
inline std::optional<LocalTimestamp> parseIsoTimestamp(const QString& value)
{
    if (value.size() == 10) {
        const auto date = parseIsoCalendarDay(value);
        if (!date) {
            return std::nullopt;
        }
        return LocalTimestamp{ *date, std::nullopt };
    }

    if (value.size() > 10
        && (value.at(10) == QLatin1Char('T') || value.at(10) == QLatin1Char(' '))) {
        const QString timeAndZone = value.mid(11);
        const bool hasOffset = timeAndZone.contains(QLatin1Char('Z'))
            || timeAndZone.contains(QLatin1Char('z'))
            || timeAndZone.contains(QLatin1Char('+'))
            || timeAndZone.contains(QLatin1Char('-'));
        if (!hasOffset) {
            const auto date = parseIsoCalendarDay(value.left(10));
            const QTime time = QTime::fromString(timeAndZone, Qt::ISODate);
            if (!date || !time.isValid()) {
                return std::nullopt;
            }
            return LocalTimestamp{ *date, time };
        }

        const QDateTime dt = QDateTime::fromString(value, Qt::ISODate);
        if (!dt.isValid()) {
            return std::nullopt;
        }
        const QDateTime local = dt.toLocalTime();
        return LocalTimestamp{ local.date(), local.time() };
    }

    return std::nullopt;
}

inline bool fallsOnLocalDay(const std::optional<QString>& value, const QDate& day)
{
    if (!value || !day.isValid()) {
        return false;
    }
    const auto parsed = parseIsoTimestamp(*value);
    return parsed.has_value() && parsed->date == day;
}

inline QString extractLocalTimeHHmm(const std::optional<QString>& value)
{
    if (!value || value->size() <= 10) {
        return QString();
    }
    const auto parsed = parseIsoTimestamp(*value);
    if (!parsed || !parsed->time) {
        return QString();
    }
    return parsed->time->toString(QStringLiteral("HH:mm"));
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
