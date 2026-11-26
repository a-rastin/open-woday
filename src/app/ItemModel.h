#ifndef OPENWODAY_APP_ITEMMODEL_H
#define OPENWODAY_APP_ITEMMODEL_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtGlobal>

#include <functional>
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
    Q_PROPERTY(bool hasDuration READ hasDuration NOTIFY selectedTaskChanged)
    Q_PROPERTY(int durationMin READ durationMin NOTIFY selectedTaskChanged)
    Q_PROPERTY(bool hasDueDate READ hasDueDate NOTIFY selectedTaskChanged)
    Q_PROPERTY(QString dueDate READ dueDate NOTIFY selectedTaskChanged)
    Q_PROPERTY(bool hasDueTime READ hasDueTime NOTIFY selectedTaskChanged)
    Q_PROPERTY(QString dueTime READ dueTime NOTIFY selectedTaskChanged)
    Q_PROPERTY(QString duePrecision READ duePrecision NOTIFY selectedTaskChanged)
    Q_PROPERTY(bool hasFolder READ hasFolder NOTIFY selectedTaskChanged)
    Q_PROPERTY(QString folderId READ folderId NOTIFY selectedTaskChanged)
    Q_PROPERTY(QString folderName READ folderName NOTIFY selectedTaskChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY selectedTaskChanged)
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)
    Q_PROPERTY(QVariantList availableTags READ availableTags NOTIFY availableTagsChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit ItemModel(Database& database, QObject* parent = nullptr);

    bool hasSelection() const;
    QString selectedId() const;
    QString title() const;
    bool completed() const;
    int priority() const;
    bool hasDuration() const;
    int durationMin() const;
    bool hasDueDate() const;
    QString dueDate() const;
    bool hasDueTime() const;
    QString dueTime() const;
    QString duePrecision() const;
    bool hasFolder() const;
    QString folderId() const;
    QString folderName() const;
    QVariantList tags() const;
    QVariantList folders() const;
    QVariantList availableTags() const;
    QString error() const;

    Q_INVOKABLE bool selectTask(const QString& id);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE bool renameTask(const QString& id, const QString& title);
    Q_INVOKABLE bool setTaskCompleted(const QString& id, bool completed);
    Q_INVOKABLE bool setTaskPriority(const QString& id, int priority);
    Q_INVOKABLE bool setTaskDuration(const QString& id, int durationMin);
    Q_INVOKABLE bool clearTaskDuration(const QString& id);
    Q_INVOKABLE bool setTaskDueDate(const QString& id, const QString& date);
    Q_INVOKABLE bool setTaskDueTime(const QString& id, const QString& time);
    Q_INVOKABLE bool setTaskDueDateTime(const QString& id,
                                        const QString& date,
                                        const QString& time);
    Q_INVOKABLE bool clearTaskDueTime(const QString& id);
    Q_INVOKABLE bool clearTaskDueDate(const QString& id);
    Q_INVOKABLE QString createFolder(const QString& name);
    Q_INVOKABLE bool setTaskFolder(const QString& id, const QString& folderId);
    Q_INVOKABLE bool createAndAssignFolder(const QString& id, const QString& name);
    Q_INVOKABLE bool clearTaskFolder(const QString& id);
    Q_INVOKABLE QString createTag(const QString& name);
    Q_INVOKABLE bool attachTag(const QString& id, const QString& tagId);
    Q_INVOKABLE bool attachTagByName(const QString& id, const QString& tagName);
    Q_INVOKABLE bool detachTag(const QString& id, const QString& tagId);
    Q_INVOKABLE bool deleteTask(const QString& id);

public slots:
    void syncTask(qlonglong id);

signals:
    void selectionChanged();
    void selectedTaskChanged();
    void foldersChanged();
    void availableTagsChanged();
    void errorChanged();
    void taskMutated(qlonglong id);

private:
    std::optional<Item> readTask(const QString& id);
    bool updateTaskField(const QString& id,
                         const std::function<bool(const Item&)>& unchanged,
                         const std::function<void(Item&, const QString& now)>& apply);
    void applySelectedItem(const Item& item);
    void refreshFolders();
    void refreshAvailableTags();
    void setError(const QString& error);

    Database& m_database;
    qlonglong m_selectedId = 0;
    QString m_title;
    bool m_completed = false;
    int m_priority = 0;
    std::optional<int> m_durationMin;
    QString m_dueDate;
    QString m_dueTime;
    QString m_duePrecision;
    std::optional<qlonglong> m_folderId;
    QString m_folderName;
    QVariantList m_tags;
    QVariantList m_folders;
    QVariantList m_availableTags;
    QString m_error;
};

#endif // OPENWODAY_APP_ITEMMODEL_H
