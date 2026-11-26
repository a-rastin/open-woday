#ifndef OPENWODAY_APP_NOTEBUFFER_H
#define OPENWODAY_APP_NOTEBUFFER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtGlobal>

class Database;

// NoteBuffer owns the debounced plain-text edit lifecycle and formatted-Block
// read-only guard for a single Item's Blocks, shared by TodayModel (Day notes)
// and ItemModel (Task Notes).
class NoteBuffer : public QObject
{
    Q_OBJECT

public:
    explicit NoteBuffer(Database& database, QObject* parent = nullptr);
    ~NoteBuffer() override;

    qlonglong itemId() const;
    QString text() const;
    bool editable() const;
    bool isDirty() const;
    QString error() const;

    bool load(qlonglong itemId);
    void reset(const QString& error = QString());
    void setText(const QString& text);
    bool flush();
    void setError(const QString& error);

signals:
    void textChanged();
    void editableChanged();
    void errorChanged();

private:
    void setEditable(bool editable);

    Database& m_database;
    qlonglong m_itemId = 0;
    QString m_text;
    QString m_savedText;
    QString m_error;
    bool m_dirty = false;
    bool m_editable = false;
    QTimer m_saveTimer;
};

#endif // OPENWODAY_APP_NOTEBUFFER_H
