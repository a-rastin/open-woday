#include "BlockRepository.h"

#include "Database.h"

#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

// Nullable-column helpers live in StoreHelpers.h, shared with
// ItemRepository, so a change to them is made once.
using store::bindOptional;
using store::readOptionalText;
using store::readOptionalId;

// The one authoritative mapping from a stored row to a Block, in the order of
// the SELECT statements. Every column of the blocks table has a place here.
Block blockFromQuery(const QSqlQuery& query)
{
    Block block;
    block.id = query.value(0).toLongLong();
    block.itemId = query.value(1).toLongLong();
    block.position = query.value(2).toInt();
    block.type = query.value(3).toString();
    block.content = query.value(4).toString();
    block.inlineJson = readOptionalText(query, 5);
    block.taskId = readOptionalId(query, 6);
    block.indent = query.value(7).toInt();
    return block;
}

} // namespace

BlockRepository::BlockRepository(Database& database)
    : StoreRepository(database)
{
}

std::optional<Block> BlockRepository::create(const Block& block)
{
    m_error.clear();

    QSqlDatabase db = m_database.connection();

    // Positions are contiguous from 0 within an Item. position < 0 asks to
    // append; an explicit position is an insertion point, clamped to the end.
    QSqlQuery extent(db);
    extent.prepare(QStringLiteral(
        "SELECT COUNT(*), COALESCE(MAX(position) + 1, 0) "
        "FROM blocks WHERE item_id = ?"));
    extent.addBindValue(block.itemId);
    if (!extent.exec() || !extent.next()) {
        fail(QStringLiteral("could not inspect the owning Item's Blocks: %1")
                 .arg(extent.lastError().text()));
        return std::nullopt;
    }
    const int existingCount = extent.value(0).toInt();
    const int appendPosition = extent.value(1).toInt();
    const bool insertAtPoint = block.position >= 0;
    const int position = insertAtPoint ? qMin(block.position, existingCount)
                                       : appendPosition;

    // Shifting and inserting commit together, so a failed create never leaves
    // a gap or a duplicate position behind.
    if (!db.transaction()) {
        fail(QStringLiteral("could not begin creating the Block: %1")
                 .arg(db.lastError().text()));
        return std::nullopt;
    }
    if (insertAtPoint && position < existingCount) {
        QSqlQuery shift(db);
        shift.prepare(QStringLiteral(
            "UPDATE blocks SET position = position + 1 "
            "WHERE item_id = ? AND position >= ?"));
        shift.addBindValue(block.itemId);
        shift.addBindValue(position);
        if (!shift.exec()) {
            fail(QStringLiteral(
                     "could not re-sequence Blocks for insertion: %1")
                     .arg(shift.lastError().text()));
            db.rollback();
            return std::nullopt;
        }
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO blocks (item_id, position, type, content, inline_json, "
        "task_id, indent) VALUES (?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(block.itemId);
    query.addBindValue(position);
    query.addBindValue(block.type);
    // A default-constructed QString is null and the SQLite driver binds it as
    // SQL NULL; content is NOT NULL, and an empty Block is a legitimate empty
    // line. Normalise it to a non-null empty string.
    query.addBindValue(block.content.isNull() ? QStringLiteral("")
                                              : block.content);
    query.addBindValue(bindOptional(block.inlineJson));
    query.addBindValue(bindOptional(block.taskId));
    query.addBindValue(block.indent);
    if (!query.exec()) {
        fail(QStringLiteral("could not create the Block: %1")
                 .arg(query.lastError().text()));
        db.rollback();
        return std::nullopt;
    }

    const QVariant insertedId = query.lastInsertId();
    if (!insertedId.isValid()) {
        fail(QStringLiteral("the created Block has no id"));
        db.rollback();
        return std::nullopt;
    }
    if (!db.commit()) {
        fail(QStringLiteral("could not commit creating the Block: %1")
                 .arg(db.lastError().text()));
        return std::nullopt;
    }

    Block stored = block;
    stored.id = insertedId.toLongLong();
    stored.position = position;
    stored.content = block.content.isNull() ? QStringLiteral("") : block.content;
    return stored;
}

QList<Block> BlockRepository::readByItem(qlonglong itemId)
{
    return select(QStringLiteral(
        "SELECT id, item_id, position, type, content, inline_json, task_id, "
        "indent FROM blocks WHERE item_id = ? ORDER BY position, id"),
        QVariantList{ QVariant(itemId) });
}

std::optional<Block> BlockRepository::readById(qlonglong id)
{
    const QList<Block> blocks = select(QStringLiteral(
        "SELECT id, item_id, position, type, content, inline_json, task_id, "
        "indent FROM blocks WHERE id = ?"),
        QVariantList{ QVariant(id) });
    if (blocks.isEmpty()) {
        return std::nullopt;
    }
    return blocks.first();
}

bool BlockRepository::reorder(qlonglong itemId,
                              const QList<qlonglong>& orderedIds)
{
    m_error.clear();

    QSqlDatabase db = m_database.connection();

    QSqlQuery existing(db);
    existing.prepare(QStringLiteral(
        "SELECT id FROM blocks WHERE item_id = ?"));
    existing.addBindValue(itemId);
    if (!existing.exec()) {
        fail(QStringLiteral("could not read the Blocks to reorder: %1")
                 .arg(existing.lastError().text()));
        return false;
    }
    QSet<qlonglong> ownIds;
    while (existing.next()) {
        ownIds.insert(existing.value(0).toLongLong());
    }
    if (orderedIds.size() != ownIds.size()) {
        fail(QStringLiteral(
                 "reorder rejected: %1 ids were given but Item %2 has %3 Blocks")
                 .arg(static_cast<qlonglong>(orderedIds.size()))
                 .arg(itemId)
                 .arg(static_cast<qlonglong>(ownIds.size())));
        return false;
    }
    QSet<qlonglong> requested;
    for (qlonglong id : orderedIds) {
        if (!ownIds.contains(id)) {
            fail(QStringLiteral(
                     "reorder rejected: Block %1 does not belong to Item %2")
                     .arg(id).arg(itemId));
            return false;
        }
        requested.insert(id);
    }
    if (requested.size() != orderedIds.size()) {
        fail(QStringLiteral("reorder rejected: a Block id was repeated"));
        return false;
    }

    // Every position is rewritten together so a failed reorder changes nothing.
    if (!db.transaction()) {
        fail(QStringLiteral("could not begin reordering Blocks: %1")
                 .arg(db.lastError().text()));
        return false;
    }
    for (int i = 0; i < orderedIds.size(); ++i) {
        QSqlQuery update(db);
        update.prepare(QStringLiteral(
            "UPDATE blocks SET position = ? WHERE id = ? AND item_id = ?"));
        update.addBindValue(i);
        update.addBindValue(orderedIds.at(i));
        update.addBindValue(itemId);
        if (!update.exec()) {
            fail(QStringLiteral("could not reorder Block %1: %2")
                     .arg(orderedIds.at(i)).arg(update.lastError().text()));
            db.rollback();
            return false;
        }
        if (update.numRowsAffected() != 1) {
            fail(QStringLiteral("reorder rejected: Block %1 could not be moved")
                     .arg(orderedIds.at(i)));
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        fail(QStringLiteral("could not commit the reorder: %1")
                 .arg(db.lastError().text()));
        return false;
    }
    return true;
}

bool BlockRepository::remove(qlonglong id)
{
    m_error.clear();

    QSqlDatabase db = m_database.connection();

    QSqlQuery lookup(db);
    lookup.prepare(QStringLiteral(
        "SELECT item_id, position FROM blocks WHERE id = ?"));
    lookup.addBindValue(id);
    if (!lookup.exec()) {
        fail(QStringLiteral("could not find Block %1: %2")
                 .arg(id).arg(lookup.lastError().text()));
        return false;
    }
    if (!lookup.next()) {
        fail(QStringLiteral("no Block with id %1 could be deleted").arg(id));
        return false;
    }
    const qlonglong itemId = lookup.value(0).toLongLong();
    const int position = lookup.value(1).toInt();

    // Delete and re-sequence together: positions stay contiguous from 0 within
    // the owning Item even if either step fails.
    if (!db.transaction()) {
        fail(QStringLiteral("could not begin deleting Block %1: %2")
                 .arg(id).arg(db.lastError().text()));
        return false;
    }

    QSqlQuery deletion(db);
    deletion.prepare(QStringLiteral("DELETE FROM blocks WHERE id = ?"));
    deletion.addBindValue(id);
    if (!deletion.exec()) {
        fail(QStringLiteral("could not delete Block %1: %2")
                 .arg(id).arg(deletion.lastError().text()));
        db.rollback();
        return false;
    }

    QSqlQuery shift(db);
    shift.prepare(QStringLiteral(
        "UPDATE blocks SET position = position - 1 "
        "WHERE item_id = ? AND position > ?"));
    shift.addBindValue(itemId);
    shift.addBindValue(position);
    if (!shift.exec()) {
        fail(QStringLiteral("could not re-sequence Blocks after deleting %1: %2")
                 .arg(id).arg(shift.lastError().text()));
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        fail(QStringLiteral("could not commit deleting Block %1: %2")
                 .arg(id).arg(db.lastError().text()));
        return false;
    }
    return true;
}

bool BlockRepository::update(const Block& block)
{
    m_error.clear();

    // Only the editable content columns are written. item_id and position are
    // deliberately left untouched: they belong to create / remove / reorder,
    // and copying them from a possibly stale or cross-Item Block here would
    // re-own the row or leave gaps/duplicates in the owning Item's contiguous
    // positions. The itemId / position carried by the Block are therefore
    // ignored, and the stored owner and position always survive the write.
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "UPDATE blocks SET type = ?, content = ?, inline_json = ?, "
        "task_id = ?, indent = ? WHERE id = ?"));
    query.addBindValue(block.type);
    query.addBindValue(block.content.isNull() ? QStringLiteral("")
                                              : block.content);
    query.addBindValue(bindOptional(block.inlineJson));
    query.addBindValue(bindOptional(block.taskId));
    query.addBindValue(block.indent);
    query.addBindValue(block.id);
    if (!query.exec()) {
        fail(QStringLiteral("could not update Block %1: %2")
                 .arg(block.id).arg(query.lastError().text()));
        return false;
    }
    if (query.numRowsAffected() != 1) {
        fail(QStringLiteral("no Block with id %1 could be updated")
                 .arg(block.id));
        return false;
    }
    return true;
}

QList<Block> BlockRepository::select(const QString& statement,
                                    const QVariantList& bindings)
{
    return selectRows<Block>(statement, bindings,
                             QStringLiteral("could not read Blocks"),
                             blockFromQuery);
}
