#ifndef OPENWODAY_APP_TODAYMODEL_H
#define OPENWODAY_APP_TODAYMODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QDate>
#include <QHash>
#include <QList>
#include <QModelIndex>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QtGlobal>
#include <functional>

class QDateTime;

class Database;

// TodayModel mediates between the declarative Today page and the Phase 2
// store. It exposes one contiguous Day strip as a list model, the selected
// date and the selected day's summary as properties, and date selection as an
// invokable method. QML binds to those values and holds no date or Item logic;
// the C++ entry point only wires the model to QML.
//
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
    Q_PROPERTY(QString dayNoteText READ dayNoteText NOTIFY dayNoteTextChanged)
    Q_PROPERTY(bool dayNoteEditable READ dayNoteEditable NOTIFY dayNoteEditableChanged)
    Q_PROPERTY(QString dayNoteError READ dayNoteError NOTIFY dayNoteErrorChanged)
    Q_PROPERTY(QString taskError READ taskError NOTIFY taskErrorChanged)
    Q_PROPERTY(QString refreshError READ refreshError NOTIFY refreshErrorChanged)
    Q_PROPERTY(QVariantList lines READ lines NOTIFY linesChanged)

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
    TodayModel(Database& database, QObject* parent,
               std::function<QDateTime()> clock);
    ~TodayModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString selectedDate() const;
    QString selectedDateLabel() const;
    QString summary() const;
    int taskCount() const;
    int eventCount() const;
    QString dayNoteText() const;
    bool dayNoteEditable() const;
    QString dayNoteError() const;
    QString taskError() const;
    QString refreshError() const;
    QVariantList lines() const;
    Q_INVOKABLE bool addTask(const QString& title);
    Q_INVOKABLE bool setTaskCompleted(const QString& id, bool completed);
    Q_INVOKABLE void setDayNoteText(const QString& text);
    Q_INVOKABLE bool flushDayNote();

    // Select one of the Day strip's dates. Dates outside the strip are ignored,
    // so the view can only move within the contiguous range around today.
    Q_INVOKABLE void selectDate(const QString& date);

signals:
    void selectedDateChanged();
    void summaryChanged();
    void dayNoteTextChanged();
    void dayNoteEditableChanged();
    void dayNoteErrorChanged();
    void taskErrorChanged();
    void refreshErrorChanged();
    void linesChanged();

private slots:
    void updateToday();

private:
    void scheduleMidnight();
    void refreshLinesAndSummary();
    bool loadDayNote(const QString& date);
    void setDayNoteError(const QString& error);
    void setTaskError(const QString& error);
    void setRefreshError(const QString& error);

    Database& m_database;
    QDate m_today;
    QList<QDate> m_days;
    QDate m_selected;
    int m_taskCount = 0;
    int m_eventCount = 0;
    qlonglong m_dayNoteId = 0;
    QString m_dayNoteText;
    QString m_savedNoteText;
    QString m_dayNoteError;
    QString m_taskError;
    QString m_refreshError;
    bool m_dayNoteDirty = false;
    bool m_dayNoteEditable = false;
    QTimer m_noteSaveTimer;
    QTimer m_midnightTimer;
    std::function<QDateTime()> m_clock;
    QVariantList m_lines;
};

#endif // OPENWODAY_APP_TODAYMODEL_H
