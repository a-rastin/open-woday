#ifndef OPENWODAY_STORE_FOLDERREPOSITORY_H
#define OPENWODAY_STORE_FOLDERREPOSITORY_H

#include "StoreHelpers.h"

#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

// One Folder: the single optional place an Item is filed (GLOSSARY.md).
struct Folder
{
    qlonglong id = 0;
    QString name;
    std::optional<QString> icon;
    std::optional<QString> cover;
    int sort = 0;
};

// FolderRepository is the store-layer API for Folders, backed by the existing
// folders table in SQLite.
class FolderRepository : public StoreRepository
{
public:
    explicit FolderRepository(Database& database);

    // Create a Folder with the given name and default attributes. Rejects an
    // empty or whitespace-only name with a clear diagnostic.
    std::optional<Folder> create(const QString& name);

    // Create a Folder from all of its fields. Rejects an empty name.
    std::optional<Folder> create(const Folder& folder);

    // Read one Folder by id, or std::nullopt when absent or on error.
    std::optional<Folder> readById(qlonglong id);

    // List all Folders ordered by sort, then id.
    QList<Folder> readAll();
};

#endif // OPENWODAY_STORE_FOLDERREPOSITORY_H
