#include "ItemModel.h"

#include "store/BlockRepository.h"
#include "store/Database.h"
#include "store/FolderRepository.h"
#include "store/ItemRepository.h"
#include "store/TagRepository.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QTime>
#include <QVariantMap>

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

QVariantMap tagToMap(const Tag& tag)
{
    return QVariantMap{
        { QStringLiteral("id"), QString::number(tag.id) },
        { QStringLiteral("name"), tag.name },
        { QStringLiteral("colour"), tag.colour.value_or(QString()) },
    };
}

} // namespace

ItemModel::ItemModel(Database& database, QObject* parent)
    : QObject(parent),
      m_database(database)
{
    m_noteSaveTimer.setSingleShot(true);
    m_noteSaveTimer.setInterval(400);
    connect(&m_noteSaveTimer, &QTimer::timeout, this, &ItemModel::flushNote);
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                this, &ItemModel::flushNote);
    }

    if (m_database.connection().isOpen()) {
        refreshFolders();
        refreshAvailableTags();
    }
}

ItemModel::~ItemModel()
{
    flushNote();
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

bool ItemModel::hasFolder() const
{
    return m_folderId.has_value() && *m_folderId > 0;
}

QString ItemModel::folderId() const
{
    return hasFolder() ? QString::number(*m_folderId) : QString();
}

QString ItemModel::folderName() const
{
    return m_folderName;
}

QVariantList ItemModel::tags() const
{
    return m_tags;
}

QVariantList ItemModel::folders() const
{
    return m_folders;
}

QVariantList ItemModel::availableTags() const
{
    return m_availableTags;
}

bool ItemModel::hasParent() const
{
    return m_parentId.has_value() && *m_parentId > 0;
}

QString ItemModel::parentId() const
{
    return hasParent() ? QString::number(*m_parentId) : QString();
}

QString ItemModel::parentTitle() const
{
    return m_parentTitle;
}

QVariantList ItemModel::subtasks() const
{
    return m_subtasks;
}

int ItemModel::subtaskDone() const
{
    return m_subtaskDone;
}

int ItemModel::subtaskTotal() const
{
    return m_subtaskTotal;
}

QString ItemModel::noteText() const
{
    return m_noteText;
}

bool ItemModel::noteEditable() const
{
    return m_noteEditable;
}

QString ItemModel::noteError() const
{
    return m_noteError;
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

    QString nextFolderName;
    if (item.folderId && *item.folderId > 0) {
        FolderRepository folders(m_database);
        const std::optional<Folder> folder = folders.readById(*item.folderId);
        if (folder)
            nextFolderName = folder->name;
    }

    QVariantList nextTags;
    TagRepository tagRepo(m_database);
    const QList<Tag> storedTags = tagRepo.readForItem(item.id);
    nextTags.reserve(storedTags.size());
    for (const Tag& tag : storedTags) {
        nextTags.append(tagToMap(tag));
    }

    ItemRepository itemRepo(m_database);
    QString nextParentTitle;
    if (item.parentId && *item.parentId > 0) {
        const std::optional<Item> parent = itemRepo.readById(*item.parentId);
        if (parent)
            nextParentTitle = parent->title;
    }

    const QList<Item> childItems = itemRepo.readByParent(item.id);
    QVariantList nextSubtasks;
    nextSubtasks.reserve(childItems.size());
    int nextSubtaskDone = 0;
    const int nextSubtaskTotal = childItems.size();
    for (const Item& child : childItems) {
        const bool childCompleted = child.completedAt.has_value();
        if (childCompleted)
            ++nextSubtaskDone;

        const QList<Item> grandChildren = itemRepo.readByParent(child.id);
        int grandDone = 0;
        for (const Item& grand : grandChildren) {
            if (grand.completedAt.has_value())
                ++grandDone;
        }

        nextSubtasks.append(QVariantMap{
            { QStringLiteral("id"), QString::number(child.id) },
            { QStringLiteral("title"), child.title },
            { QStringLiteral("completed"), childCompleted },
            { QStringLiteral("priority"), child.priority },
            { QStringLiteral("subtaskDone"), grandDone },
            { QStringLiteral("subtaskTotal"), grandChildren.size() },
        });
    }

    const bool taskChanged = idChanged
        || m_title != item.title
        || m_completed != nextCompleted
        || m_priority != item.priority
        || m_durationMin != item.durationMin
        || m_dueDate != due.date
        || m_dueTime != due.time
        || m_duePrecision != due.precision
        || m_folderId != item.folderId
        || m_folderName != nextFolderName
        || m_tags != nextTags
        || m_parentId != item.parentId
        || m_parentTitle != nextParentTitle
        || m_subtasks != nextSubtasks
        || m_subtaskDone != nextSubtaskDone
        || m_subtaskTotal != nextSubtaskTotal;

    m_selectedId = item.id;
    m_title = item.title;
    m_completed = nextCompleted;
    m_priority = item.priority;
    m_durationMin = item.durationMin;
    m_dueDate = due.date;
    m_dueTime = due.time;
    m_duePrecision = due.precision;
    m_folderId = item.folderId;
    m_folderName = nextFolderName;
    m_tags = nextTags;
    m_parentId = item.parentId;
    m_parentTitle = nextParentTitle;
    m_subtasks = nextSubtasks;
    m_subtaskDone = nextSubtaskDone;
    m_subtaskTotal = nextSubtaskTotal;

    if (idChanged)
        emit selectionChanged();
    if (taskChanged)
        emit selectedTaskChanged();
}

void ItemModel::refreshSelectedTask()
{
    if (m_selectedId <= 0)
        return;

    ItemRepository items(m_database);
    const std::optional<Item> stored = items.readById(m_selectedId);
    if (!items.lastError().isEmpty())
        return;

    if (!stored || stored->kind != kTaskKind) {
        clearSelectionState();
        return;
    }

    applySelectedItem(*stored);
}

void ItemModel::refreshFolders()
{
    FolderRepository folderRepo(m_database);
    const QList<Folder> stored = folderRepo.readAll();
    if (!folderRepo.lastError().isEmpty()) {
        setError(folderRepo.lastError());
        return;
    }

    QVariantList nextFolders;
    nextFolders.reserve(stored.size());
    for (const Folder& folder : stored) {
        nextFolders.append(QVariantMap{
            { QStringLiteral("id"), QString::number(folder.id) },
            { QStringLiteral("name"), folder.name },
        });
    }

    if (m_folders != nextFolders) {
        m_folders = nextFolders;
        emit foldersChanged();
    }
}

void ItemModel::refreshAvailableTags()
{
    TagRepository tagRepo(m_database);
    const QList<Tag> stored = tagRepo.readAll();
    if (!tagRepo.lastError().isEmpty()) {
        setError(tagRepo.lastError());
        return;
    }

    QVariantList nextTags;
    nextTags.reserve(stored.size());
    for (const Tag& tag : stored) {
        nextTags.append(tagToMap(tag));
    }

    if (m_availableTags != nextTags) {
        m_availableTags = nextTags;
        emit availableTagsChanged();
    }
}

void ItemModel::setNoteEditable(bool editable)
{
    if (m_noteEditable != editable) {
        m_noteEditable = editable;
        emit noteEditableChanged();
    }
}

void ItemModel::setNoteError(const QString& error)
{
    if (m_noteError != error) {
        m_noteError = error;
        emit noteErrorChanged();
    }
}

void ItemModel::resetTaskNote()
{
    m_noteSaveTimer.stop();
    m_noteDirty = false;
    m_savedNoteText.clear();
    if (!m_noteText.isEmpty()) {
        m_noteText.clear();
        emit noteTextChanged();
    }
    setNoteEditable(false);
    setNoteError({});
}

bool ItemModel::loadTaskNote(qlonglong taskId)
{
    BlockRepository blocks(m_database);
    const QList<Block> stored = blocks.readByItem(taskId);
    if (!blocks.lastError().isEmpty()) {
        m_noteSaveTimer.stop();
        m_noteDirty = false;
        m_savedNoteText.clear();
        if (!m_noteText.isEmpty()) {
            m_noteText.clear();
            emit noteTextChanged();
        }
        setNoteEditable(false);
        setNoteError(blocks.lastError());
        return false;
    }

    QStringList lines;
    lines.reserve(stored.size());
    bool editable = true;
    for (const Block& block : stored) {
        lines.append(block.content);
        editable = editable && BlockRepository::isPlainText(block);
    }

    const QString nextText = lines.join(QLatin1Char('\n'));
    m_noteSaveTimer.stop();
    m_noteDirty = false;
    m_savedNoteText = nextText;
    if (m_noteText != nextText) {
        m_noteText = nextText;
        emit noteTextChanged();
    }
    setNoteEditable(editable);
    setNoteError(editable
                     ? QString()
                     : QStringLiteral(
                         "This Note contains formatted Blocks and is read-only "
                         "in the plain-text editor."));
    return true;
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

    if (m_selectedId > 0 && m_noteDirty && !flushNote()) {
        if (stored->id != m_selectedId) {
            setNoteError(QStringLiteral(
                "Cannot switch Task because the Task Note could not be saved. "
                "Your changes are still here; try again. %1").arg(m_noteError));
        }
        return false;
    }

    setError({});
    loadTaskNote(stored->id);
    applySelectedItem(*stored);
    return true;
}

void ItemModel::clearSelection()
{
    if (m_selectedId > 0 && m_noteDirty && !flushNote()) {
        setNoteError(QStringLiteral(
            "Cannot close Task details because the Task Note could not be saved. "
            "Your changes are still here; try again. %1").arg(m_noteError));
        return;
    }
    clearSelectionState();
}

void ItemModel::clearSelectionState()
{
    const bool hadSelection = (m_selectedId != 0);
    const bool hadTaskData = hadSelection || !m_title.isEmpty() || m_completed
        || m_priority != 0 || m_durationMin.has_value()
        || !m_dueDate.isEmpty() || !m_dueTime.isEmpty() || !m_duePrecision.isEmpty()
        || m_folderId.has_value() || !m_folderName.isEmpty() || !m_tags.isEmpty()
        || m_parentId.has_value() || !m_parentTitle.isEmpty() || !m_subtasks.isEmpty()
        || m_subtaskDone != 0 || m_subtaskTotal != 0;

    m_selectedId = 0;
    m_title.clear();
    m_completed = false;
    m_priority = 0;
    m_durationMin = std::nullopt;
    m_dueDate.clear();
    m_dueTime.clear();
    m_duePrecision.clear();
    m_folderId = std::nullopt;
    m_folderName.clear();
    m_tags.clear();
    m_parentId = std::nullopt;
    m_parentTitle.clear();
    m_subtasks.clear();
    m_subtaskDone = 0;
    m_subtaskTotal = 0;
    resetTaskNote();
    setError({});

    if (hadSelection)
        emit selectionChanged();
    if (hadTaskData)
        emit selectedTaskChanged();
}

QString ItemModel::addSubtask(const QString& parentId, const QString& title)
{
    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty()) {
        setError(QStringLiteral("Subtask title cannot be empty."));
        return QString();
    }

    const std::optional<Item> parent = readTask(parentId);
    if (!parent)
        return QString();

    Item subtask;
    subtask.kind = kTaskKind;
    subtask.title = trimmed;
    subtask.parentId = parent->id;

    ItemRepository items(m_database);
    const std::optional<Item> created = items.create(subtask);
    if (!created) {
        setError(items.lastError());
        return QString();
    }

    setError({});
    refreshSelectedTask();
    emit taskMutated(created->id);
    return QString::number(created->id);
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
    else
        refreshSelectedTask();
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
        else
            refreshSelectedTask();
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
    else
        refreshSelectedTask();
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
    else
        refreshSelectedTask();
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

QString ItemModel::createFolder(const QString& name)
{
    FolderRepository folderRepo(m_database);
    const std::optional<Folder> created = folderRepo.create(name);
    if (!created) {
        setError(folderRepo.lastError());
        return QString();
    }

    setError({});
    refreshFolders();
    return QString::number(created->id);
}

bool ItemModel::setTaskFolder(const QString& id, const QString& folderId)
{
    if (folderId.trimmed().isEmpty())
        return clearTaskFolder(id);

    bool valid = false;
    const qlonglong parsedFolderId = folderId.toLongLong(&valid);
    if (!valid || parsedFolderId <= 0) {
        setError(QStringLiteral("The selected Folder identifier is invalid."));
        return false;
    }

    FolderRepository folderRepo(m_database);
    const std::optional<Folder> folder = folderRepo.readById(parsedFolderId);
    if (!folderRepo.lastError().isEmpty()) {
        setError(folderRepo.lastError());
        return false;
    }
    if (!folder) {
        setError(QStringLiteral("The selected Folder could not be found."));
        return false;
    }

    return updateTaskField(
        id,
        [parsedFolderId](const Item& item) {
            return item.folderId == std::optional<qlonglong>(parsedFolderId);
        },
        [parsedFolderId](Item& updated, const QString& now) {
            updated.folderId = parsedFolderId;
            updated.modifiedAt = now;
        });
}

bool ItemModel::createAndAssignFolder(const QString& id, const QString& name)
{
    if (!readTask(id))
        return false;

    const QString createdId = createFolder(name);
    if (createdId.isEmpty())
        return false;

    return setTaskFolder(id, createdId);
}

bool ItemModel::clearTaskFolder(const QString& id)
{
    return updateTaskField(
        id,
        [](const Item& item) { return !item.folderId.has_value(); },
        [](Item& updated, const QString& now) {
            updated.folderId = std::nullopt;
            updated.modifiedAt = now;
        });
}

QString ItemModel::createTag(const QString& name)
{
    TagRepository tagRepo(m_database);
    const std::optional<Tag> tag = tagRepo.findOrCreate(name);
    if (!tag) {
        setError(tagRepo.lastError());
        return QString();
    }

    setError({});
    refreshAvailableTags();
    return QString::number(tag->id);
}

bool ItemModel::attachTag(const QString& id, const QString& tagId)
{
    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    bool valid = false;
    const qlonglong parsedTagId = tagId.toLongLong(&valid);
    if (!valid || parsedTagId <= 0) {
        setError(QStringLiteral("The selected Tag identifier is invalid."));
        return false;
    }

    TagRepository tagRepo(m_database);
    const std::optional<Tag> tag = tagRepo.readById(parsedTagId);
    if (!tagRepo.lastError().isEmpty()) {
        setError(tagRepo.lastError());
        return false;
    }
    if (!tag) {
        setError(QStringLiteral("The selected Tag could not be found."));
        return false;
    }

    if (!tagRepo.attachToItem(stored->id, parsedTagId)) {
        setError(tagRepo.lastError());
        return false;
    }

    setError({});
    if (stored->id == m_selectedId)
        applySelectedItem(*stored);
    emit taskMutated(stored->id);
    return true;
}

bool ItemModel::attachTagByName(const QString& id, const QString& tagName)
{
    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    const QString trimmed = tagName.trimmed();
    if (trimmed.isEmpty()) {
        setError(QStringLiteral("Tag name cannot be empty."));
        return false;
    }

    TagRepository tagRepo(m_database);
    const std::optional<Tag> tag = tagRepo.findOrCreate(trimmed);
    if (!tag) {
        setError(tagRepo.lastError());
        return false;
    }

    if (!tagRepo.attachToItem(stored->id, tag->id)) {
        setError(tagRepo.lastError());
        return false;
    }

    setError({});
    refreshAvailableTags();
    if (stored->id == m_selectedId)
        applySelectedItem(*stored);
    emit taskMutated(stored->id);
    return true;
}

bool ItemModel::detachTag(const QString& id, const QString& tagId)
{
    const std::optional<Item> stored = readTask(id);
    if (!stored)
        return false;

    bool valid = false;
    const qlonglong parsedTagId = tagId.toLongLong(&valid);
    if (!valid || parsedTagId <= 0) {
        setError(QStringLiteral("The selected Tag identifier is invalid."));
        return false;
    }

    TagRepository tagRepo(m_database);
    if (!tagRepo.detachFromItem(stored->id, parsedTagId)) {
        setError(tagRepo.lastError());
        return false;
    }

    setError({});
    if (stored->id == m_selectedId)
        applySelectedItem(*stored);
    emit taskMutated(stored->id);
    return true;
}

void ItemModel::setNoteText(const QString& text)
{
    if (!m_noteEditable || m_selectedId <= 0 || text == m_noteText)
        return;

    m_noteText = text;
    emit noteTextChanged();
    if (text == m_savedNoteText) {
        m_noteSaveTimer.stop();
        m_noteDirty = false;
        setNoteError({});
        return;
    }

    m_noteDirty = true;
    m_noteSaveTimer.start();
}

bool ItemModel::flushNote()
{
    m_noteSaveTimer.stop();
    if (!m_noteDirty || m_selectedId <= 0)
        return true;

    BlockRepository blocks(m_database);
    if (!blocks.replaceText(m_selectedId, m_noteText)) {
        setNoteError(blocks.lastError());
        return false;
    }

    m_noteDirty = false;
    m_savedNoteText = m_noteText;
    setNoteError({});
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
        clearSelectionState();
    else
        refreshSelectedTask();
    emit taskMutated(stored->id);
    return true;
}

void ItemModel::syncTask(qlonglong id)
{
    if (m_selectedId == 0)
        return;

    if (id != m_selectedId) {
        refreshSelectedTask();
        return;
    }

    ItemRepository items(m_database);
    const std::optional<Item> stored = items.readById(id);
    if (!items.lastError().isEmpty())
        return;

    if (!stored || stored->kind != kTaskKind) {
        clearSelectionState();
        return;
    }

    applySelectedItem(*stored);
}
