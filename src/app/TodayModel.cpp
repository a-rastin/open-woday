#include "TodayModel.h"

#include "store/Database.h"
#include "store/ItemRepository.h"

#include <QLocale>

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
      m_selected(m_today),
      m_selectedDate(isoDate(m_today))
{
    m_days.reserve(kDayStripSize);
    for (int i = 0; i < kDayStripSize; ++i) {
        m_days.append(m_today.addDays(i - kDaysBeforeToday));
    }
    refreshSummary();
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
        return isoDate(date) == m_selectedDate;
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
    return m_selectedDate;
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

void TodayModel::selectDate(const QString& date)
{
    const QDate parsed = QDate::fromString(date, Qt::ISODate);
    if (!parsed.isValid() || !m_days.contains(parsed)) {
        return;
    }

    const QString normalized = isoDate(parsed);
    if (normalized == m_selectedDate) {
        return;
    }

    m_selected = parsed;
    m_selectedDate = normalized;
    emit selectedDateChanged();
    if (!m_days.isEmpty()) {
        emit dataChanged(index(0), index(m_days.size() - 1),
                         QList<int>{ IsSelectedRole });
    }
    refreshSummary();
}

void TodayModel::refreshSummary()
{
    // The store is the single source of truth: normal reads exclude
    // soft-deleted Items, so the counts describe the selected day's live Items.
    ItemRepository items(m_database);
    int tasks = 0;
    int events = 0;
    const QList<Item> taskItems = items.readByKind(kTaskKind);
    if (!items.lastError().isEmpty()) {
        qWarning("TodayModel: %s", qPrintable(items.lastError()));
    }
    for (const Item& item : taskItems) {
        if (item.dueAt && item.dueAt->left(10) == m_selectedDate) {
            ++tasks;
        }
    }
    const QList<Item> eventItems = items.readByKind(kEventKind);
    if (!items.lastError().isEmpty()) {
        qWarning("TodayModel: %s", qPrintable(items.lastError()));
    }
    for (const Item& item : eventItems) {
        if (item.startAt && item.startAt->left(10) == m_selectedDate) {
            ++events;
        }
    }

    m_taskCount = tasks;
    m_eventCount = events;
    emit summaryChanged();
}
