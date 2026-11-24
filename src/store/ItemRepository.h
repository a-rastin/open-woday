#ifndef OPENWODAY_STORE_ITEMREPOSITORY_H
#define OPENWODAY_STORE_ITEMREPOSITORY_H

#include <QString>
#include <QtGlobal>

#include <optional>

class Database;

// One Item as it is stored. A Task, Note or Event is one Item of a particular
// kind (GLOSSARY.md). This slice persists the fields a single Task Item needs;
// later slices extend the same Item.
struct Item
{
    qlonglong id = 0;
    QString kind;       // 'task', 'note' or 'event'
    QString title;
    QString createdAt;  // ISO 8601
    QString modifiedAt; // ISO 8601
};

// ItemRepository is the store-layer API for Items. It writes straight to the
// single local SQLite file owned by Database; there is no in-memory cache, so
// a written Item survives a restart of the connection.
class ItemRepository
{
public:
    explicit ItemRepository(Database& database);

    // Create one Item of the given kind and return it with its assigned id and
    // timestamps. Returns std::nullopt and sets lastError() on failure.
    std::optional<Item> create(const QString& kind, const QString& title);

    // Read an Item by id. Returns std::nullopt when it is absent or on failure.
    std::optional<Item> readById(qlonglong id);

    // Clear, human-readable diagnostic for a failed create()/readById().
    QString lastError() const;

private:
    void fail(const QString& reason);

    Database& m_database;
    QString m_error;
};

#endif // OPENWODAY_STORE_ITEMREPOSITORY_H
