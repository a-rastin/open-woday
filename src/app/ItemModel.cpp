#include "ItemModel.h"

#include "store/Database.h"
#include "store/ItemRepository.h"

#include <QDate>
#include <QDateTime>
#include <QTime>

namespace {

const QString kTaskKind = QStringLiteral("task");
const QString kDuePrecisionDay = QStringLiteral("day");
const QString kDuePrecisionTime = QStringLiteral("time");

struct DueFields {
    QString date;
    QString time;
    QString precision;
};

QString nowIsoUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

std::optional<QDate> parseIsoCalendarDay(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.size() != 10)
        return std::nullopt;
    const QDate date = QDate::fromString(trimmed, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid() || date.toString(Qt::ISODate) != trimmed)
        return std::nullopt;
    return date;
}

std::optional<QTime> parseIso24HourTime(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.size() != 5 || trimmed.at(2) != QLatin1Char(':'))
        return std::nullopt;
    const QTime time = QTime::fromString(trimmed, QStringLiteral("HH:mm"));
    if (!time.isValid() || time.toString(QStringLiteral("HH:mm")) != trimmed)
        return std::nullopt;
    return time;
}

QString formatDueDateTime(const QDate& day, const QTime& time)
{
    return day.toString(Qt::ISODate) + QLatin1Char('T')
        + time.toString(QStringLiteral("HH:mm:00"));
}

DueFields extractDueFields(const Item& item)
{
    DueFields fields;
    if (!item.dueAt || item.dueAt->isEmpty())
        return fields;

    const QString precision = item.duePrecision.value_or(QString());
    if (item.dueAt->size() == 10) {
        if (const auto day = parseIsoCalendarDay(*item.dueAt)) {
            fields.date = day->toString(Qt::ISODate);
            fields.precision = precision.isEmpty() ? kDuePrecisionDay : precision;
        }
        return fields;
    }

    const QDateTime dt = QDateTime::fromString(*item.dueAt, Qt::ISODate);
    if (!dt.isValid())
        return fields;

    const QDateTime local = dt.toLocalTime();
    fields.date = local.date().toString(Qt::ISODate);
    fields.precision = precision;
    if (precision == kDuePrecisionTime)
        fields.time = local.time().toString(QStringLiteral("HH:mm"));
    return fields;
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

bool ItemModel::hasDuration() const
{
    return m_durationMin.has_value() && *m_durationMin > 0;
}

int ItemModel::durationMin() const
{
    return hasDuration() ? *m_durationMin : 0;
}

bool ItemModel::hasDueDate() const
{
    return !m_dueDate.isEmpty();
}

QString ItemModel::dueDate() const
{
    return m_dueDate;
}

bool ItemModel::hasDueTime() const
{
    return hasDueDate() && m_duePrecision == kDuePrecisionTime && !m_dueTime.isEmpty();
}

QString ItemModel::dueTime() const
{
    return hasDueTime() ? m_dueTime : QString();
}

QString ItemModel::duePrecision() const
{
    return m_duePrecision;
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
    const DueFields due = extractDueFields(item);
    const bool taskChanged = idChanged
        || m_title != item.title
        || m_completed != nextCompleted
        || m_priority != item.priority
        || m_durationMin != item.durationMin
        || m_dueDate != due.date
        || m_dueTime != due.time
        || m_duePrecision != due.precision;

    m_selectedId = item.id;
    m_title = item.title;
    m_completed = nextCompleted;
    m_priority = item.priority;
    m_durationMin = item.durationMin;
    m_dueDate = due.date;
    m_dueTime = due.time;
    m_duePrecision = due.precision;

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
        || m_priority != 0 || m_durationMin.has_value()
        || !m_dueDate.isEmpty() || !m_dueTime.isEmpty() || !m_duePrecision.isEmpty();

    m_selectedId = 0;
    m_title.clear();
    m_completed = false;
    m_priority = 0;
    m_durationMin = std::nullopt;
    m_dueDate.clear();
    m_dueTime.clear();
    m_duePrecision.clear();
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

bool ItemModel::updateTaskField(
    const QString& id,
    const std::function<bool(const Item&)>& unchanged,
    const std::function<void(Item&, const QString& now)>& apply)
{
    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    if (unchanged(*stored)) {
        setError({});
        if (stored->id == m_selectedId)
            applySelectedItem(*stored);
        return true;
    }

    Item updated = *stored;
    apply(updated, nowIsoUtc());

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

bool ItemModel::setTaskPriority(const QString& id, int priority)
{
    if (priority < 0 || priority > 3) {
        setError(QStringLiteral("Task priority must be None, Low, Medium, or High."));
        return false;
    }

    return updateTaskField(
        id,
        [priority](const Item& item) { return item.priority == priority; },
        [priority](Item& updated, const QString& now) {
            updated.priority = priority;
            updated.modifiedAt = now;
        });
}

bool ItemModel::setTaskDuration(const QString& id, int durationMin)
{
    if (durationMin <= 0) {
        setError(QStringLiteral("Planned duration must be a positive number of minutes."));
        return false;
    }

    return updateTaskField(
        id,
        [durationMin](const Item& item) {
            return item.durationMin == std::optional<int>(durationMin);
        },
        [durationMin](Item& updated, const QString& now) {
            updated.durationMin = durationMin;
            updated.modifiedAt = now;
        });
}

bool ItemModel::clearTaskDuration(const QString& id)
{
    return updateTaskField(
        id,
        [](const Item& item) { return !item.durationMin.has_value(); },
        [](Item& updated, const QString& now) {
            updated.durationMin = std::nullopt;
            updated.modifiedAt = now;
        });
}

bool ItemModel::setTaskDueDate(const QString& id, const QString& date)
{
    const auto parsed = parseIsoCalendarDay(date);
    if (!parsed) {
        setError(QStringLiteral("Due date must be a valid calendar date (YYYY-MM-DD)."));
        return false;
    }

    const QString isoDay = parsed->toString(Qt::ISODate);
    return updateTaskField(
        id,
        [isoDay](const Item& item) {
            return item.dueAt == std::optional<QString>(isoDay)
                && item.duePrecision == std::optional<QString>(kDuePrecisionDay);
        },
        [isoDay](Item& updated, const QString& now) {
            updated.dueAt = isoDay;
            updated.duePrecision = kDuePrecisionDay;
            updated.modifiedAt = now;
        });
}

bool ItemModel::setTaskDueTime(const QString& id, const QString& time)
{
    const auto parsedTime = parseIso24HourTime(time);
    if (!parsedTime) {
        setError(QStringLiteral("Due time must be a 24-hour time (HH:mm)."));
        return false;
    }

    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    const DueFields currentDue = extractDueFields(*stored);
    if (currentDue.date.isEmpty()) {
        setError(QStringLiteral("Set a due date before adding a due time."));
        return false;
    }

    return setTaskDueDateTime(id, currentDue.date,
                              parsedTime->toString(QStringLiteral("HH:mm")));
}

bool ItemModel::setTaskDueDateTime(const QString& id,
                                   const QString& date,
                                   const QString& time)
{
    if (time.trimmed().isEmpty())
        return setTaskDueDate(id, date);

    const auto parsedDate = parseIsoCalendarDay(date);
    if (!parsedDate) {
        setError(QStringLiteral("Due date must be a valid calendar date (YYYY-MM-DD)."));
        return false;
    }

    const auto parsedTime = parseIso24HourTime(time);
    if (!parsedTime) {
        setError(QStringLiteral("Due time must be a 24-hour time (HH:mm)."));
        return false;
    }

    const QString isoDateTime = formatDueDateTime(*parsedDate, *parsedTime);
    return updateTaskField(
        id,
        [isoDateTime](const Item& item) {
            return item.dueAt == std::optional<QString>(isoDateTime)
                && item.duePrecision == std::optional<QString>(kDuePrecisionTime);
        },
        [isoDateTime](Item& updated, const QString& now) {
            updated.dueAt = isoDateTime;
            updated.duePrecision = kDuePrecisionTime;
            updated.modifiedAt = now;
        });
}

bool ItemModel::clearTaskDueTime(const QString& id)
{
    return updateTaskField(
        id,
        [](const Item& item) {
            return !item.dueAt.has_value()
                || item.duePrecision != std::optional<QString>(kDuePrecisionTime);
        },
        [](Item& updated, const QString& now) {
            const DueFields currentDue = extractDueFields(updated);
            if (currentDue.date.isEmpty()) {
                updated.dueAt = std::nullopt;
                updated.duePrecision = std::nullopt;
            } else {
                updated.dueAt = currentDue.date;
                updated.duePrecision = kDuePrecisionDay;
            }
            updated.modifiedAt = now;
        });
}

bool ItemModel::clearTaskDueDate(const QString& id)
{
    return updateTaskField(
        id,
        [](const Item& item) {
            return !item.dueAt.has_value() && !item.duePrecision.has_value();
        },
        [](Item& updated, const QString& now) {
            updated.dueAt = std::nullopt;
            updated.duePrecision = std::nullopt;
            updated.modifiedAt = now;
        });
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
