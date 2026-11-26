#ifndef OPENWODAY_STORE_TAGREPOSITORY_H
#define OPENWODAY_STORE_TAGREPOSITORY_H

#include "StoreHelpers.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

// One Tag: a label attachable to any Item, crossing Folders (GLOSSARY.md).
struct Tag
{
    qlonglong id = 0;
    QString name;
    std::optional<QString> colour;
    std::optional<QString> icon;
};

// TagRepository is the store-layer API for Tags and Item-Tag associations,
// backed by the existing tags and item_tags tables in SQLite.
class TagRepository : public StoreRepository
{
public:
    explicit TagRepository(Database& database);

    // Create a Tag with a unique name. Fails cleanly on empty or duplicate names.
    std::optional<Tag> create(const QString& name,
                              const std::optional<QString>& colour = std::nullopt,
                              const std::optional<QString>& icon = std::nullopt);

    // Look up a Tag by its unique name, or return std::nullopt if absent.
    std::optional<Tag> findByName(const QString& name);

    // Return the existing Tag matching name, or create it if absent.
    std::optional<Tag> findOrCreate(const QString& name,
                                    const std::optional<QString>& colour = std::nullopt,
                                    const std::optional<QString>& icon = std::nullopt);

    // Read one Tag by id, or std::nullopt when absent or on error.
    std::optional<Tag> readById(qlonglong id);

    // List all Tags ordered alphabetically by name, then id.
    QList<Tag> readAll();

    // Attach a Tag to an Item in item_tags. Idempotent if already attached;
    // fails loudly on foreign-key or database errors.
    bool attachToItem(qlonglong itemId, qlonglong tagId);

    // Detach a Tag from an Item in item_tags.
    bool detachFromItem(qlonglong itemId, qlonglong tagId);

    // Read all Tags attached to a given Item, ordered by name, then id.
    QList<Tag> readForItem(qlonglong itemId);

    // Read all Tags attached to any of the given Items in bulk, keyed by
    // item_id. Each Item's list is ordered by name, then id.
    QHash<qlonglong, QList<Tag>> readForItems(const QList<qlonglong>& itemIds);

private:
    std::optional<Tag> insertTag(const QString& trimmedName,
                                 const std::optional<QString>& colour,
                                 const std::optional<QString>& icon);
};

#endif // OPENWODAY_STORE_TAGREPOSITORY_H
