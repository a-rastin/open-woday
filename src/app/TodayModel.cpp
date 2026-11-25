#include "TodayModel.h"

#include "store/Database.h"
#include "store/BlockRepository.h"
#include "store/ItemRepository.h"

#include <QLocale>
#include <QDateTime>

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

} // namespace

TodayModel::TodayModel(Database& database, QObject* parent)
    : QAbstractListModel(parent),
      m_database(database),
      m_today(QDate::currentDate()),
      m_selected(m_today)
{
    m_days.reserve(kDayStripSize);
    for (int i = 0; i < kDayStripSize; ++i) {
        m_days.append(m_today.addDays(i - kDaysBeforeToday));
    }
    refreshLinesAndSummary();
    loadDayNote(selectedDate());
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
    return m_dayNoteText;
}

QString TodayModel::dayNoteError() const
{
    return m_dayNoteError;
}

QVariantList TodayModel::lines() const
{
    return m_lines;
}

void TodayModel::addTask(const QString& title)
{
    if (title.trimmed().isEmpty())
        return;
    Item item;
    item.kind = kTaskKind;
    item.title = title.trimmed();
    item.dueAt = selectedDate();
    item.duePrecision = QStringLiteral("day");
    ItemRepository items(m_database);
    if (!items.create(item)) {
        setDayNoteError(items.lastError());
        return;
    }
    refreshLinesAndSummary();
}

bool TodayModel::setTaskCompleted(const QString& id, bool completed)
{
    bool valid = false;
    const qlonglong itemId = id.toLongLong(&valid);
    if (!valid)
        return false;
    ItemRepository items(m_database);
    const auto stored = items.readById(itemId);
    if (!stored || stored->kind != kTaskKind || !stored->dueAt
        || stored->dueAt->left(10) != selectedDate()) {
        if (!items.lastError().isEmpty())
            setDayNoteError(items.lastError());
        return false;
    }
    if (stored->completedAt.has_value() == completed) {
        refreshLinesAndSummary();
        return true;
    }
    Item updated = *stored;
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    updated.completedAt = completed ? std::optional<QString>(now) : std::nullopt;
    updated.modifiedAt = now;
    if (!items.update(updated)) {
        setDayNoteError(items.lastError());
        return false;
    }
    refreshLinesAndSummary();
    return true;
}

void TodayModel::setDayNoteError(const QString& error)
{
    if (m_dayNoteError != error) {
        m_dayNoteError = error;
        emit dayNoteErrorChanged();
    }
}

bool TodayModel::loadDayNote(const QString& date)
{
    ItemRepository items(m_database);
    const QList<Item> dated = items.readByDay(date);
    if (!items.lastError().isEmpty()) {
        setDayNoteError(items.lastError());
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
            setDayNoteError(items.lastError());
            return false;
        }
        id = created->id;
    }
    BlockRepository blocks(m_database);
    const QList<Block> stored = blocks.readByItem(id);
    if (!blocks.lastError().isEmpty()) {
        setDayNoteError(blocks.lastError());
        return false;
    }
    QStringList lines;
    for (const Block& block : stored) {
        lines.append(block.content);
    }
    m_dayNoteId = id;
    m_dayNoteText = lines.join(QLatin1Char('\n'));
    m_savedNoteText = m_dayNoteText;
    m_dayNoteDirty = false;
    setDayNoteError({});
    emit dayNoteTextChanged();
    return true;
}

void TodayModel::setDayNoteText(const QString& text)
{
    if (!m_dayNoteId || (text == m_dayNoteText && !m_dayNoteDirty)) {
        return;
    }
    m_dayNoteText = text;
    if (text == m_savedNoteText) {
        m_dayNoteDirty = false;
        setDayNoteError({});
        emit dayNoteTextChanged();
        return;
    }
    m_dayNoteDirty = true;
    BlockRepository blocks(m_database);
    if (!blocks.replaceText(m_dayNoteId, text)) {
        setDayNoteError(blocks.lastError());
        return;
    }
    m_dayNoteDirty = false;
    m_savedNoteText = text;
    setDayNoteError({});
    emit dayNoteTextChanged();
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

    if (m_dayNoteDirty) {
        setDayNoteText(m_dayNoteText);
        if (m_dayNoteDirty)
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

void TodayModel::refreshLinesAndSummary()
{
    // The store is the single source of truth: normal reads exclude
    // soft-deleted Items, so the counts describe the selected day's live Items.
    ItemRepository items(m_database);
    int tasks = 0;
    int events = 0;
    QVariantList lines;
    const QList<Item> taskItems = items.readByKind(kTaskKind);
    if (!items.lastError().isEmpty()) {
        qWarning("TodayModel: %s", qPrintable(items.lastError()));
    }
    for (const Item& item : taskItems) {
        if (item.dueAt && item.dueAt->left(10) == selectedDate()) {
            ++tasks;
            lines.append(QVariantMap{
                {QStringLiteral("id"), QString::number(item.id)},
                {QStringLiteral("title"), item.title},
                {QStringLiteral("completed"), item.completedAt.has_value()},
            });
        }
    }
    const QList<Item> eventItems = items.readByKind(kEventKind);
    if (!items.lastError().isEmpty()) {
        qWarning("TodayModel: %s", qPrintable(items.lastError()));
    }
    for (const Item& item : eventItems) {
        if (item.startAt && item.startAt->left(10) == selectedDate()) {
            ++events;
        }
    }

    m_taskCount = tasks;
    m_eventCount = events;
    m_lines = lines;
    emit linesChanged();
    emit summaryChanged();
}
