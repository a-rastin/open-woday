#include "FolderRepository.h"

#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

using store::bindOptional;
using store::readOptionalText;

Folder folderFromQuery(const QSqlQuery& query)
{
    Folder folder;
    folder.id = query.value(0).toLongLong();
    folder.name = query.value(1).toString();
    folder.icon = readOptionalText(query, 2);
    folder.cover = readOptionalText(query, 3);
    folder.sort = query.value(4).toInt();
    return folder;
}

} // namespace

FolderRepository::FolderRepository(Database& database)
    : StoreRepository(database)
{
}

std::optional<Folder> FolderRepository::create(const QString& name)
{
    Folder folder;
    folder.name = name;
    return create(folder);
}

std::optional<Folder> FolderRepository::create(const Folder& folder)
{
    m_error.clear();
    const QString trimmed = folder.name.trimmed();
    if (trimmed.isEmpty()) {
        fail(QStringLiteral("Folder name cannot be empty."));
        return std::nullopt;
    }

    Folder stored = folder;
    stored.name = trimmed;

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO folders (name, icon, cover, sort) VALUES (?, ?, ?, ?)"));
    query.addBindValue(stored.name);
    query.addBindValue(bindOptional(stored.icon));
    query.addBindValue(bindOptional(stored.cover));
    query.addBindValue(stored.sort);
    if (!query.exec()) {
        fail(QStringLiteral("could not create the Folder: %1")
                 .arg(query.lastError().text()));
        return std::nullopt;
    }

    const QVariant insertedId = query.lastInsertId();
    if (!insertedId.isValid()) {
        fail(QStringLiteral("the created Folder has no id"));
        return std::nullopt;
    }

    stored.id = insertedId.toLongLong();
    return stored;
}

std::optional<Folder> FolderRepository::readById(qlonglong id)
{
    const QList<Folder> folders = selectRows<Folder>(
        QStringLiteral("SELECT id, name, icon, cover, sort FROM folders WHERE id = ?"),
        QVariantList{ QVariant(id) },
        QStringLiteral("could not read Folders"),
        folderFromQuery);
    if (folders.isEmpty()) {
        return std::nullopt;
    }
    return folders.first();
}

QList<Folder> FolderRepository::readAll()
{
    return selectRows<Folder>(
        QStringLiteral("SELECT id, name, icon, cover, sort FROM folders ORDER BY sort, id"),
        {},
        QStringLiteral("could not read Folders"),
        folderFromQuery);
}
