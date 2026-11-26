#include "ItemModel.h"

#include "store/Database.h"
#include "store/ItemRepository.h"

#include <QDateTime>

namespace {

const QString kTaskKind = QStringLiteral("task");

QString nowIsoUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

} // namespace

ItemModel::ItemModel(Database& database, QObject* parent)
    : QObject(parent),
      m_database(database)
{
}

bool ItemModel::hasSelection() const
{
    return m_selectedId > 0;
}

QString ItemModel::selectedId() const
{
    return m_selectedId > 0 ? QString::number(m_selectedId) : QString();
}

QString ItemModel::title() const
{
    return m_title;
}

bool ItemModel::completed() const
{
    return m_completed;
}

int ItemModel::priority() const
{
    return m_priority;
}

QString ItemModel::error() const
{
    return m_error;
}

std::optional<Item> ItemModel::readTask(const QString& id)
{
    bool valid = false;
    const qlonglong itemId = id.toLongLong(&valid);
    if (!valid || itemId <= 0) {
        setError(QStringLiteral("The selected Task identifier is invalid."));
        return std::nullopt;
    }

    ItemRepository items(m_database);
    const std::optional<Item> stored = items.readById(itemId);
    if (!items.lastError().isEmpty()) {
        setError(items.lastError());
        return std::nullopt;
    }
    if (!stored || stored->kind != kTaskKind) {
        setError(QStringLiteral("The selected Task could not be found."));
        return std::nullopt;
    }
    return stored;
}

void ItemModel::applySelectedItem(const Item& item)
{
    const bool idChanged = (m_selectedId != item.id);
    const bool nextCompleted = item.completedAt.has_value();
    const bool taskChanged = idChanged
        || m_title != item.title
        || m_completed != nextCompleted
        || m_priority != item.priority;

    m_selectedId = item.id;
    m_title = item.title;
    m_completed = nextCompleted;
    m_priority = item.priority;

    if (idChanged)
        emit selectionChanged();
    if (taskChanged)
        emit selectedTaskChanged();
}

void ItemModel::setError(const QString& error)
{
    if (m_error != error) {
        m_error = error;
        emit errorChanged();
    }
}

bool ItemModel::selectTask(const QString& id)
{
    if (id.trimmed().isEmpty()) {
        clearSelection();
        return false;
    }

    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    setError({});
    applySelectedItem(*stored);
    return true;
}

void ItemModel::clearSelection()
{
    const bool hadSelection = (m_selectedId != 0);
    const bool hadTaskData = hadSelection || !m_title.isEmpty() || m_completed
        || m_priority != 0;

    m_selectedId = 0;
    m_title.clear();
    m_completed = false;
    m_priority = 0;
    setError({});

    if (hadSelection)
        emit selectionChanged();
    if (hadTaskData)
        emit selectedTaskChanged();
}

bool ItemModel::renameTask(const QString& id, const QString& title)
{
    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty()) {
        setError(QStringLiteral("Task title cannot be empty."));
        return false;
    }

    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    if (stored->title == trimmed) {
        setError({});
        if (stored->id == m_selectedId)
            applySelectedItem(*stored);
        return true;
    }

    Item updated = *stored;
    updated.title = trimmed;
    updated.modifiedAt = nowIsoUtc();

    ItemRepository items(m_database);
    if (!items.update(updated)) {
        setError(items.lastError());
        return false;
    }

    setError({});
    if (updated.id == m_selectedId)
        applySelectedItem(updated);
    emit taskMutated(updated.id);
    return true;
}

bool ItemModel::setTaskCompleted(const QString& id, bool completed)
{
    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    if (stored->completedAt.has_value() == completed) {
        setError({});
        if (stored->id == m_selectedId)
            applySelectedItem(*stored);
        emit taskMutated(stored->id);
        return true;
    }

    Item updated = *stored;
    const QString now = nowIsoUtc();
    updated.completedAt = completed ? std::optional<QString>(now) : std::nullopt;
    updated.modifiedAt = now;

    ItemRepository items(m_database);
    if (!items.update(updated)) {
        setError(items.lastError());
        return false;
    }

    setError({});
    if (updated.id == m_selectedId)
        applySelectedItem(updated);
    emit taskMutated(updated.id);
    return true;
}

bool ItemModel::deleteTask(const QString& id)
{
    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    ItemRepository items(m_database);
    if (!items.softDelete(stored->id)) {
        setError(items.lastError());
        return false;
    }

    setError({});
    if (stored->id == m_selectedId)
        clearSelection();
    emit taskMutated(stored->id);
    return true;
}

void ItemModel::syncTask(qlonglong id)
{
    if (m_selectedId == 0 || id != m_selectedId)
        return;

    ItemRepository items(m_database);
    const std::optional<Item> stored = items.readById(id);
    if (!items.lastError().isEmpty())
        return;

    if (!stored || stored->kind != kTaskKind) {
        clearSelection();
        return;
    }

    applySelectedItem(*stored);
}
