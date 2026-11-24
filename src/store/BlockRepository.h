#ifndef OPENWODAY_STORE_BLOCKREPOSITORY_H
#define OPENWODAY_STORE_BLOCKREPOSITORY_H

#include <QList>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <optional>

class Database;

// One Block: one line of a Note's content (GLOSSARY.md). A to-do Block is a
// Block type, not a synonym for a Task; taskId optionally points at the real
// Task Item that a to-do Block represents. Blocks are rows in their own table,
// so per-Block editing and that foreign key stay simple.
struct Block
{
    qlonglong id = 0;
    qlonglong itemId = 0;                // owning Item (a Note)
    int position = -1;                   // -1 asks create() to append
    QString type;                        // 'text','heading','todo','bullet','code'
    QString content;                     // plain UTF-8 text
    std::optional<QString> inlineJson;   // bold/italic/link formatting ranges
    std::optional<qlonglong> taskId;     // to-do Block -> Task id
    int indent = 0;
};

// BlockRepository is the store-layer API for Blocks. It writes straight to the
// single local SQLite file owned by Database; there is no in-memory cache, so
// a written Block survives a restart of the connection.
class BlockRepository
{
public:
    explicit BlockRepository(Database& database);

    // Append one Block to its owning Item's Blocks by default (position = -1).
    // An explicit position inserts there instead, shifting the Blocks already
    // present so positions stay contiguous from 0; a position past the end
    // clamps to an append. Returns the stored Block with its assigned id and
    // position, or std::nullopt and sets lastError() on failure.
    std::optional<Block> create(const Block& block);

    // Blocks of one Item, ordered by position within that Item.
    QList<Block> readByItem(qlonglong itemId);

    // One Block by id, or std::nullopt when absent.
    std::optional<Block> readById(qlonglong id);

    // Re-sequence the owning Item's Blocks into the given id order. Returns
    // false and sets lastError() if the ids do not describe exactly this
    // Item's Blocks; a rejected reorder changes nothing.
    bool reorder(qlonglong itemId, const QList<qlonglong>& orderedIds);

    // Delete one Block and re-sequence the positions of the Blocks that
    // remain in its owning Item. Returns false and sets lastError() on failure.
    bool remove(qlonglong id);

    // Write every stored column of an existing Block, identified by its id.
    // Positions are managed by create / remove / reorder instead.
    // Returns false and sets lastError() on failure.
    bool update(const Block& block);

    // Clear, human-readable diagnostic for a failed operation.
    QString lastError() const;

private:
    // Execute one fixed SELECT statement with bound values and map every row
    // through blockFromQuery(). No SQL is assembled from Block text.
    QList<Block> select(const QString& statement,
                        const QVariantList& bindings);
    void fail(const QString& reason);

    Database& m_database;
    QString m_error;
};

#endif // OPENWODAY_STORE_BLOCKREPOSITORY_H
