#include "TagRepository.h"

#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

using store::bindOptional;
using store::readOptionalText;

Tag tagFromQuery(const QSqlQuery& query)
{
    Tag tag;
    tag.id = query.value(0).toLongLong();
    tag.name = query.value(1).toString();
    tag.colour = readOptionalText(query, 2);
    tag.icon = readOptionalText(query, 3);
    return tag;
}

} // namespace

TagRepository::TagRepository(Database& database)
    : StoreRepository(database)
{
}

std::optional<Tag> TagRepository::create(const QString& name,
                                         const std::optional<QString>& colour,
                                         const std::optional<QString>& icon)
{
    m_error.clear();
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        fail(QStringLiteral("Tag name cannot be empty."));
        return std::nullopt;
    }

    const std::optional<Tag> existing = findByName(trimmed);
    if (!m_error.isEmpty()) {
        return std::nullopt;
    }
    if (existing) {
        fail(QStringLiteral("a Tag named '%1' already exists")
                 .arg(existing->name));
        return std::nullopt;
    }

    return insertTag(trimmed, colour, icon);
}

std::optional<Tag> TagRepository::findByName(const QString& name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        m_error.clear();
        return std::nullopt;
    }

    const QList<Tag> tags = selectRows<Tag>(
        QStringLiteral("SELECT id, name, colour, icon FROM tags "
                       "WHERE name = ? COLLATE NOCASE "
                       "ORDER BY id"),
        QVariantList{ QVariant(trimmed) },
        QStringLiteral("could not look up the Tag"),
        tagFromQuery);
    if (tags.isEmpty()) {
        return std::nullopt;
    }
    return tags.first();
}

std::optional<Tag> TagRepository::findOrCreate(const QString& name,
                                               const std::optional<QString>& colour,
                                               const std::optional<QString>& icon)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        fail(QStringLiteral("Tag name cannot be empty."));
        return std::nullopt;
    }

    const std::optional<Tag> existing = findByName(trimmed);
    if (!m_error.isEmpty()) {
        return std::nullopt;
    }
    if (existing) {
        return existing;
    }
    return insertTag(trimmed, colour, icon);
}

std::optional<Tag> TagRepository::insertTag(const QString& trimmedName,
                                            const std::optional<QString>& colour,
                                            const std::optional<QString>& icon)
{
    m_error.clear();

    Tag stored;
    stored.name = trimmedName;
    stored.colour = colour;
    stored.icon = icon;

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO tags (name, colour, icon) VALUES (?, ?, ?)"));
    query.addBindValue(stored.name);
    query.addBindValue(bindOptional(stored.colour));
    query.addBindValue(bindOptional(stored.icon));
    if (!query.exec()) {
        const QString dbError = query.lastError().text();
        if (dbError.contains(QStringLiteral("UNIQUE constraint failed"))) {
            fail(QStringLiteral("a Tag named '%1' already exists: %2")
                     .arg(stored.name, dbError));
        } else {
            fail(QStringLiteral("could not create the Tag: %1").arg(dbError));
        }
        return std::nullopt;
    }

    const QVariant insertedId = query.lastInsertId();
    if (!insertedId.isValid()) {
        fail(QStringLiteral("the created Tag has no id"));
        return std::nullopt;
    }

    stored.id = insertedId.toLongLong();
    return stored;
}

std::optional<Tag> TagRepository::readById(qlonglong id)
{
    const QList<Tag> tags = selectRows<Tag>(
        QStringLiteral("SELECT id, name, colour, icon FROM tags WHERE id = ?"),
        QVariantList{ QVariant(id) },
        QStringLiteral("could not read Tags"),
        tagFromQuery);
    if (tags.isEmpty()) {
        return std::nullopt;
    }
    return tags.first();
}

QList<Tag> TagRepository::readAll()
{
    return selectRows<Tag>(
        QStringLiteral("SELECT id, name, colour, icon FROM tags "
                       "ORDER BY name COLLATE NOCASE, id"),
        {},
        QStringLiteral("could not read Tags"),
        tagFromQuery);
}

bool TagRepository::attachToItem(qlonglong itemId, qlonglong tagId)
{
    m_error.clear();

    QSqlQuery checkQuery(m_database.connection());
    checkQuery.prepare(QStringLiteral(
        "SELECT 1 FROM item_tags WHERE item_id = ? AND tag_id = ?"));
    checkQuery.addBindValue(itemId);
    checkQuery.addBindValue(tagId);
    if (!checkQuery.exec()) {
        fail(QStringLiteral("could not check Tag attachment: %1")
                 .arg(checkQuery.lastError().text()));
        return false;
    }
    if (checkQuery.next()) {
        return true;
    }

    QSqlQuery insertQuery(m_database.connection());
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO item_tags (item_id, tag_id) VALUES (?, ?)"));
    insertQuery.addBindValue(itemId);
    insertQuery.addBindValue(tagId);
    if (!insertQuery.exec()) {
        fail(QStringLiteral("could not attach Tag %1 to Item %2: %3")
                 .arg(tagId).arg(itemId).arg(insertQuery.lastError().text()));
        return false;
    }
    return true;
}

bool TagRepository::detachFromItem(qlonglong itemId, qlonglong tagId)
{
    m_error.clear();

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "DELETE FROM item_tags WHERE item_id = ? AND tag_id = ?"));
    query.addBindValue(itemId);
    query.addBindValue(tagId);
    if (!query.exec()) {
        fail(QStringLiteral("could not detach Tag %1 from Item %2: %3")
                 .arg(tagId).arg(itemId).arg(query.lastError().text()));
        return false;
    }
    return true;
}

QList<Tag> TagRepository::readForItem(qlonglong itemId)
{
    return selectRows<Tag>(
        QStringLiteral("SELECT tags.id, tags.name, tags.colour, tags.icon "
                       "FROM tags "
                       "JOIN item_tags ON item_tags.tag_id = tags.id "
                       "WHERE item_tags.item_id = ? "
                       "ORDER BY tags.name COLLATE NOCASE, tags.id"),
        QVariantList{ QVariant(itemId) },
        QStringLiteral("could not read Tags for Item"),
        tagFromQuery);
}
