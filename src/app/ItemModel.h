#ifndef OPENWODAY_APP_ITEMMODEL_H
#define OPENWODAY_APP_ITEMMODEL_H

#include <QObject>
#include <QString>
#include <QtGlobal>

#include <optional>

class Database;
struct Item;

// ItemModel mediates active Task selection and Task lifecycle mutations (inline
// rename, completion/reopening, and soft-delete) between declarative QML and
// the Phase 2 SQLite store. QML holds no SQL or business logic; TodayModel
// stays synchronized through signals wired in the application entry point.
class ItemModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY selectionChanged)
    Q_PROPERTY(QString title READ title NOTIFY selectedTaskChanged)
    Q_PROPERTY(bool completed READ completed NOTIFY selectedTaskChanged)
    Q_PROPERTY(int priority READ priority NOTIFY selectedTaskChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit ItemModel(Database& database, QObject* parent = nullptr);

    bool hasSelection() const;
    QString selectedId() const;
    QString title() const;
    bool completed() const;
    int priority() const;
    QString error() const;

    Q_INVOKABLE bool selectTask(const QString& id);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE bool renameTask(const QString& id, const QString& title);
    Q_INVOKABLE bool setTaskCompleted(const QString& id, bool completed);
    Q_INVOKABLE bool deleteTask(const QString& id);

public slots:
    void syncTask(qlonglong id);

signals:
    void selectionChanged();
    void selectedTaskChanged();
    void errorChanged();
    void taskMutated(qlonglong id);

private:
    std::optional<Item> readTask(const QString& id);
    void applySelectedItem(const Item& item);
    void setError(const QString& error);

    Database& m_database;
    qlonglong m_selectedId = 0;
    QString m_title;
    bool m_completed = false;
    int m_priority = 0;
    QString m_error;
};

#endif // OPENWODAY_APP_ITEMMODEL_H
