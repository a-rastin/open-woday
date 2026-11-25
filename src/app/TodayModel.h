#ifndef OPENWODAY_APP_TODAYMODEL_H
#define OPENWODAY_APP_TODAYMODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QDate>
#include <QHash>
#include <QList>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QtGlobal>

class Database;

// TodayModel mediates between the declarative Today page and the Phase 2
// store. It exposes one contiguous Day strip as a list model, the selected
// date and the selected day's summary as properties, and date selection as an
// invokable method. QML binds to those values and holds no date or Item logic;
// the C++ entry point only wires the model to QML.
//
// No Item is written here: the one Day note per date belongs to a later slice.
// The summary counts are derived afresh from the selected date's Items read
// through ItemRepository, so they read zero until Task and Event lines exist.
class TodayModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedDate READ selectedDate NOTIFY selectedDateChanged)
    Q_PROPERTY(QString selectedDateLabel READ selectedDateLabel
               NOTIFY selectedDateChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
    Q_PROPERTY(int taskCount READ taskCount NOTIFY summaryChanged)
    Q_PROPERTY(int eventCount READ eventCount NOTIFY summaryChanged)

public:
    // Roles of one day cell in the Day strip. The model owns the date
    // arithmetic and the selection state; the view only renders them.
    enum DayRole {
        DateRole = Qt::UserRole + 1,
        DayNumberRole,
        WeekdayRole,
        IsTodayRole,
        IsSelectedRole,
    };
    Q_ENUM(DayRole)

    explicit TodayModel(Database& database, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString selectedDate() const;
    QString selectedDateLabel() const;
    QString summary() const;
    int taskCount() const;
    int eventCount() const;

    // Select one of the Day strip's dates. Dates outside the strip are ignored,
    // so the view can only move within the contiguous range around today.
    Q_INVOKABLE void selectDate(const QString& date);

signals:
    void selectedDateChanged();
    void summaryChanged();

private:
    void refreshSummary();

    Database& m_database;
    QDate m_today;
    QList<QDate> m_days;
    QDate m_selected;
    QString m_selectedDate;
    int m_taskCount = 0;
    int m_eventCount = 0;
};

#endif // OPENWODAY_APP_TODAYMODEL_H
