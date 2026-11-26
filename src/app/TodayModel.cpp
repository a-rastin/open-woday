#include "TodayModel.h"

#include "store/Database.h"
#include "store/FolderRepository.h"
#include "store/ItemRepository.h"
#include "store/StoreHelpers.h"
#include "store/TagRepository.h"

#include <QLocale>
#include <QDateTime>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QHash>
#include <QSet>
#include <QTimeZone>
#include <utility>

namespace {

// The Day strip holds roughly ten consecutive days, with today a little before
// its middle so recent and upcoming dates are both one selection away.
constexpr int kDayStripSize = 10;
constexpr int kDaysBeforeToday = 4;

const QString kTaskKind = QStringLiteral("task");
const QString kEventKind = QStringLiteral("event");

// Dates are carried as 'YYYY-MM-DD', the shape of items.day (schema.sql).
QString isoDate(const QDate& date)
{
    return date.toString(Qt::ISODate);
}

bool isScheduledForOtherDay(const Item& item, const QSet<qlonglong>& dueIds)
{
    return item.dueAt && !item.dueAt->isEmpty() && !dueIds.contains(item.id);
}

bool hasDueAncestor(const Item& item,
                    const QHash<qlonglong, Item>& taskById,
                    const QSet<qlonglong>& dueIds)
{
    QSet<qlonglong> visited;
    visited.insert(item.id);
    std::optional<qlonglong> cursor = item.parentId;
    while (cursor && *cursor > 0 && !visited.contains(*cursor)) {
        if (dueIds.contains(*cursor))
            return true;
        visited.insert(*cursor);
        const auto parentIt = taskById.constFind(*cursor);
        if (parentIt == taskById.constEnd())
            break;
        // If an ancestor was explicitly scheduled on another day, it is not
        // rendered on the selected day, so its subtree is cut here and
        // descendants due today remain visible at root level.
        if (isScheduledForOtherDay(*parentIt, dueIds))
            break;
        cursor = parentIt->parentId;
    }
    return false;
}

bool readItemTagList(TagRepository& tagRepo,
                     qlonglong itemId,
                     QVariantList* outTags,
                     QString* outError)
{
    const QList<Tag> itemTags = tagRepo.readForItem(itemId);
    if (!tagRepo.lastError().isEmpty()) {
        *outError = tagRepo.lastError();
        return false;
    }
    QVariantList tagList;
    tagList.reserve(itemTags.size());
    for (const Tag& tag : itemTags) {
        tagList.append(QVariantMap{
            {QStringLiteral("id"), QString::number(tag.id)},
            {QStringLiteral("name"), tag.name},
            {QStringLiteral("colour"), tag.colour.value_or(QString())},
        });
    }
    *outTags = tagList;
    return true;
}

bool buildTaskLineMap(const Item& item,
                      const QHash<qlonglong, QList<Item>>& childrenByParent,
                      const QHash<qlonglong, QString>& folderNames,
                      TagRepository& tagRepo,
                      QSet<qlonglong>& visited,
                      QVariantMap* outMap,
                      QString* outError)
{
    visited.insert(item.id);

    const QString dueTime = (item.duePrecision == QStringLiteral("time"))
        ? store::extractLocalTimeHHmm(item.dueAt)
        : QString();

    QVariantList tagList;
    if (!readItemTagList(tagRepo, item.id, &tagList, outError))
        return false;

    const QString folderId = (item.folderId && *item.folderId > 0)
        ? QString::number(*item.folderId)
        : QString();
    const QString folderName = (item.folderId && *item.folderId > 0)
        ? folderNames.value(*item.folderId)
        : QString();

    const QList<Item> childItems = childrenByParent.value(item.id);
    QVariantList subtaskLines;
    subtaskLines.reserve(childItems.size());
    int subtaskDone = 0;
    const int subtaskTotal = childItems.size();
    for (const Item& child : childItems) {
        if (child.completedAt.has_value())
            ++subtaskDone;
        if (visited.contains(child.id))
            continue;
        QVariantMap childMap;
        if (!buildTaskLineMap(child, childrenByParent, folderNames, tagRepo,
                              visited, &childMap, outError)) {
            return false;
        }
        subtaskLines.append(childMap);
    }

    *outMap = QVariantMap{
        {QStringLiteral("id"), QString::number(item.id)},
        {QStringLiteral("kind"), kTaskKind},
        {QStringLiteral("title"), item.title},
        {QStringLiteral("completed"), item.completedAt.has_value()},
        {QStringLiteral("priority"), item.priority},
        {QStringLiteral("durationMin"), item.durationMin.value_or(0)},
        {QStringLiteral("dueTime"), dueTime},
        {QStringLiteral("folderId"), folderId},
        {QStringLiteral("folderName"), folderName},
        {QStringLiteral("tags"), tagList},
        {QStringLiteral("parentId"),
         (item.parentId && *item.parentId > 0) ? QString::number(*item.parentId) : QString()},
        {QStringLiteral("subtaskDone"), subtaskDone},
        {QStringLiteral("subtaskTotal"), subtaskTotal},
        {QStringLiteral("subtasks"), subtaskLines},
    };
    return true;
}

} // namespace

TodayModel::TodayModel(Database& database, QObject* parent)
    : TodayModel(database, parent, [] { return QDateTime::currentDateTime(); })
{
}

TodayModel::TodayModel(Database& database, QObject* parent,
                       std::function<QDateTime()> clock)
    : QAbstractListModel(parent),
      m_database(database),
      m_today(clock().date()),
      m_selected(m_today),
      m_dayNote(database, this),
      m_clock(std::move(clock))
{
    connect(&m_dayNote, &NoteBuffer::textChanged,
            this, &TodayModel::dayNoteTextChanged);
    connect(&m_dayNote, &NoteBuffer::editableChanged,
            this, &TodayModel::dayNoteEditableChanged);
    connect(&m_dayNote, &NoteBuffer::errorChanged,
            this, &TodayModel::dayNoteErrorChanged);

    m_days.reserve(kDayStripSize);
    for (int i = 0; i < kDayStripSize; ++i) {
        m_days.append(m_today.addDays(i - kDaysBeforeToday));
    }
    refreshLinesAndSummary();
    loadDayNote(selectedDate());
    m_midnightTimer.setSingleShot(true);
    m_midnightTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_midnightTimer, &QTimer::timeout, this, &TodayModel::updateToday);
    if (auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        connect(app, &QGuiApplication::applicationStateChanged, this,
                [this](Qt::ApplicationState state) {
                    if (state == Qt::ApplicationActive)
                        updateToday();
                });
    }
    scheduleMidnight();
}

void TodayModel::scheduleMidnight()
{
    const QDateTime now = m_clock();
    const QDateTime midnight = now.date().addDays(1).startOfDay(now.timeZone());
    m_midnightTimer.start(static_cast<int>(
        qBound(qint64(1), now.msecsTo(midnight), qint64(60000))));
}

void TodayModel::updateToday()
{
    const QDate today = m_clock().date();
    if (today != m_today) {
        const bool followToday = m_selected == m_today;
        const QDate first = today.addDays(-kDaysBeforeToday);
        const QDate last = first.addDays(kDayStripSize - 1);
        const bool outside = m_selected < first || m_selected > last;
        bool selectionChanged = false;
        if (followToday || outside) {
            const QDate target = followToday ? today
                : (m_selected < first ? first : last);
            if (flushDayNote() && loadDayNote(isoDate(target))) {
                m_selected = target;
                selectionChanged = true;
            } else if (m_dayNote.isDirty()) {
                m_dayNote.setError(QStringLiteral(
                    "Cannot change date because the Day note could not be saved. "
                    "Your changes are still here; try again. %1").arg(m_dayNote.error()));
            }
        }

        // If saving or loading the replacement failed, keep a contiguous strip
        // containing the still-open note instead of leaving it unreachable.
        QDate stripFirst = first;
        if (m_selected < first)
            stripFirst = m_selected;
        else if (m_selected > last)
            stripFirst = m_selected.addDays(1 - kDayStripSize);
        beginResetModel();
        m_today = today;
        m_days.clear();
        for (int i = 0; i < kDayStripSize; ++i)
            m_days.append(stripFirst.addDays(i));
        endResetModel();
        if (selectionChanged) {
            emit selectedDateChanged();
            refreshLinesAndSummary();
        }
    }
    scheduleMidnight();
}

TodayModel::~TodayModel()
{
    flushDayNote();
}

int TodayModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_days.size();
}

QVariant TodayModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= m_days.size()) {
        return {};
    }

    const QDate date = m_days.at(index.row());
    switch (role) {
    case DateRole:
        return isoDate(date);
    case DayNumberRole:
        return QString::number(date.day());
    case WeekdayRole:
        return QLocale::c().dayName(date.dayOfWeek(),
                                    QLocale::ShortFormat);
    case IsTodayRole:
        return date == m_today;
    case IsSelectedRole:
        return date == m_selected;
    default:
        return {};
    }
}

QHash<int, QByteArray> TodayModel::roleNames() const
{
    return {
        { DateRole, "date" },
        { DayNumberRole, "dayNumber" },
        { WeekdayRole, "weekday" },
        { IsTodayRole, "isToday" },
        { IsSelectedRole, "isSelected" },
    };
}

QString TodayModel::selectedDate() const
{
    return isoDate(m_selected);
}

QString TodayModel::selectedDateLabel() const
{
    return QLocale::c().toString(m_selected,
                                 QStringLiteral("dddd, d MMMM yyyy"));
}

QString TodayModel::summary() const
{
    // An original open-woday string carrying real derived counts. With no Task
    // or Event lines yet, both counts are zero.
    const QString tasks = m_taskCount == 1 ? QStringLiteral("task")
                                           : QStringLiteral("tasks");
    const QString events = m_eventCount == 1 ? QStringLiteral("event")
                                             : QStringLiteral("events");
    return QStringLiteral("%1 %2 and %3 %4 on this day")
        .arg(QString::number(m_taskCount))
        .arg(tasks)
        .arg(QString::number(m_eventCount))
        .arg(events);
}

int TodayModel::taskCount() const
{
    return m_taskCount;
}

int TodayModel::eventCount() const
{
    return m_eventCount;
}

QString TodayModel::dayNoteText() const
{
    return m_dayNote.text();
}

bool TodayModel::dayNoteEditable() const
{
    return m_dayNote.editable();
}

QString TodayModel::dayNoteError() const
{
    return m_dayNote.error();
}

QString TodayModel::taskError() const
{
    return m_taskError;
}

QString TodayModel::refreshError() const
{
    return m_refreshError;
}

QVariantList TodayModel::lines() const
{
    return m_lines;
}

bool TodayModel::addTask(const QString& title)
{
    if (title.trimmed().isEmpty())
        return false;
    Item item;
    item.kind = kTaskKind;
    item.title = title.trimmed();
    item.dueAt = selectedDate();
    item.duePrecision = QStringLiteral("day");
    ItemRepository items(m_database);
    if (!items.create(item)) {
        setTaskError(items.lastError());
        return false;
    }
    setTaskError({});
    refreshLinesAndSummary();
    return true;
}

bool TodayModel::setTaskCompleted(const QString& id, bool completed)
{
    bool valid = false;
    const qlonglong itemId = id.toLongLong(&valid);
    if (!valid)
        return false;
    ItemRepository items(m_database);
    const auto stored = items.readById(itemId);
    if (!stored || stored->kind != kTaskKind
        || !store::fallsOnLocalDay(stored->dueAt, m_selected)) {
        if (!items.lastError().isEmpty())
            setTaskError(items.lastError());
        return false;
    }
    if (stored->completedAt.has_value() == completed) {
        setTaskError({});
        refreshLinesAndSummary();
        emit taskUpdated(itemId);
        return true;
    }
    if (!items.setCompleted(*stored, completed)) {
        setTaskError(items.lastError());
        return false;
    }
    setTaskError({});
    refreshLinesAndSummary();
    emit taskUpdated(itemId);
    return true;
}

void TodayModel::setTaskError(const QString& error)
{
    if (m_taskError != error) {
        m_taskError = error;
        emit taskErrorChanged();
    }
}

void TodayModel::setRefreshError(const QString& error)
{
    if (m_refreshError != error) {
        m_refreshError = error;
        emit refreshErrorChanged();
    }
}

bool TodayModel::loadDayNote(const QString& date)
{
    ItemRepository items(m_database);
    const QList<Item> dated = items.readByDay(date);
    if (!items.lastError().isEmpty()) {
        m_dayNote.setError(items.lastError());
        return false;
    }
    qlonglong id = 0;
    for (const Item& item : dated) {
        if (item.kind == QStringLiteral("note")) {
            id = item.id;
            break;
        }
    }
    if (!id) {
        Item note;
        note.kind = QStringLiteral("note");
        note.day = date;
        const auto created = items.create(note);
        if (!created) {
            m_dayNote.setError(items.lastError());
            return false;
        }
        id = created->id;
    }
    return m_dayNote.load(id);
}

void TodayModel::setDayNoteText(const QString& text)
{
    m_dayNote.setText(text);
}

bool TodayModel::flushDayNote()
{
    return m_dayNote.flush();
}

void TodayModel::selectDate(const QString& date)
{
    const QDate parsed = QDate::fromString(date, Qt::ISODate);
    if (!parsed.isValid() || !m_days.contains(parsed)) {
        return;
    }

    const QString normalized = isoDate(parsed);
    if (parsed == m_selected) {
        return;
    }

    if (!flushDayNote()) {
        m_dayNote.setError(QStringLiteral(
            "Cannot change date because the Day note could not be saved. "
            "Your changes are still here; try again. %1").arg(m_dayNote.error()));
        return;
    }
    if (!loadDayNote(normalized)) {
        return;
    }
    m_selected = parsed;
    emit selectedDateChanged();
    if (!m_days.isEmpty()) {
        emit dataChanged(index(0), index(m_days.size() - 1),
                         QList<int>{ IsSelectedRole });
    }
    refreshLinesAndSummary();
}

void TodayModel::refresh()
{
    refreshLinesAndSummary();
}

void TodayModel::refreshLinesAndSummary()
{
    // The store is the single source of truth: normal reads exclude
    // soft-deleted Items, so the counts describe the selected day's live Items.
    ItemRepository items(m_database);
    const QList<Item> taskItems = items.readTasksDueOn(selectedDate());
    if (!items.lastError().isEmpty()) {
        setRefreshError(items.lastError());
        return;
    }
    const QList<Item> eventItems = items.readEventsStartingOn(selectedDate());
    if (!items.lastError().isEmpty()) {
        setRefreshError(items.lastError());
        return;
    }
    const QList<Item> allTasks = items.readByKind(kTaskKind);
    if (!items.lastError().isEmpty()) {
        setRefreshError(items.lastError());
        return;
    }

    QSet<qlonglong> dueIds;
    dueIds.reserve(taskItems.size());
    for (const Item& item : taskItems)
        dueIds.insert(item.id);

    QHash<qlonglong, Item> taskById;
    QHash<qlonglong, QList<Item>> childrenByParent;
    taskById.reserve(allTasks.size());
    bool needsFolders = false;
    for (const Item& task : allTasks) {
        taskById.insert(task.id, task);
        if (task.parentId && *task.parentId > 0
            && !isScheduledForOtherDay(task, dueIds)) {
            childrenByParent[*task.parentId].append(task);
        }
        if (task.folderId && *task.folderId > 0)
            needsFolders = true;
    }
    for (const Item& eventItem : eventItems) {
        if (eventItem.folderId && *eventItem.folderId > 0)
            needsFolders = true;
    }

    QList<Item> rootTasks;
    rootTasks.reserve(taskItems.size());
    for (const Item& item : taskItems) {
        if (!hasDueAncestor(item, taskById, dueIds))
            rootTasks.append(item);
    }

    QHash<qlonglong, QString> folderNames;
    if (needsFolders) {
        FolderRepository folderRepo(m_database);
        const QList<Folder> folders = folderRepo.readAll();
        if (!folderRepo.lastError().isEmpty()) {
            setRefreshError(folderRepo.lastError());
            return;
        }
        for (const Folder& folder : folders)
            folderNames.insert(folder.id, folder.name);
    }

    TagRepository tagRepo(m_database);
    QVariantList lines;
    QSet<qlonglong> visited;
    for (const Item& item : rootTasks) {
        if (visited.contains(item.id))
            continue;
        QVariantMap lineMap;
        QString tagError;
        if (!buildTaskLineMap(item, childrenByParent, folderNames, tagRepo,
                              visited, &lineMap, &tagError)) {
            setRefreshError(tagError);
            return;
        }
        lines.append(lineMap);
    }
    for (const Item& item : eventItems) {
        QVariantList tagList;
        QString tagError;
        if (!readItemTagList(tagRepo, item.id, &tagList, &tagError)) {
            setRefreshError(tagError);
            return;
        }
        const QString folderId = (item.folderId && *item.folderId > 0)
            ? QString::number(*item.folderId)
            : QString();
        const QString folderName = (item.folderId && *item.folderId > 0)
            ? folderNames.value(*item.folderId)
            : QString();
        lines.append(QVariantMap{
            {QStringLiteral("id"), QString::number(item.id)},
            {QStringLiteral("kind"), kEventKind},
            {QStringLiteral("title"), item.title},
            {QStringLiteral("allDay"), item.allDay},
            {QStringLiteral("startTime"),
             item.allDay ? QString() : store::extractLocalTimeHHmm(item.startAt)},
            {QStringLiteral("endTime"),
             item.allDay ? QString() : store::extractLocalTimeHHmm(item.endAt)},
            {QStringLiteral("priority"), item.priority},
            {QStringLiteral("durationMin"), item.durationMin.value_or(0)},
            {QStringLiteral("folderId"), folderId},
            {QStringLiteral("folderName"), folderName},
            {QStringLiteral("tags"), tagList},
        });
    }

    m_taskCount = rootTasks.size();
    m_eventCount = eventItems.size();
    m_lines = lines;
    setRefreshError({});
    emit linesChanged();
    emit summaryChanged();
}
