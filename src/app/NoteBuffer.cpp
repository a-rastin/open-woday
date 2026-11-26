#include "NoteBuffer.h"

#include "store/BlockRepository.h"
#include "store/Database.h"

#include <QCoreApplication>
#include <QStringList>

NoteBuffer::NoteBuffer(Database& database, QObject* parent)
    : QObject(parent),
      m_database(database)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &NoteBuffer::flush);
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                this, &NoteBuffer::flush);
    }
}

NoteBuffer::~NoteBuffer()
{
    flush();
}

qlonglong NoteBuffer::itemId() const
{
    return m_itemId;
}

QString NoteBuffer::text() const
{
    return m_text;
}

bool NoteBuffer::editable() const
{
    return m_editable;
}

bool NoteBuffer::isDirty() const
{
    return m_dirty;
}

QString NoteBuffer::error() const
{
    return m_error;
}

void NoteBuffer::setEditable(bool editable)
{
    if (m_editable != editable) {
        m_editable = editable;
        emit editableChanged();
    }
}

void NoteBuffer::setError(const QString& error)
{
    if (m_error != error) {
        m_error = error;
        emit errorChanged();
    }
}

bool NoteBuffer::load(qlonglong itemId)
{
    BlockRepository blocks(m_database);
    const QList<Block> stored = blocks.readByItem(itemId);
    if (!blocks.lastError().isEmpty()) {
        setError(blocks.lastError());
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
    m_saveTimer.stop();
    m_dirty = false;
    m_itemId = itemId;
    m_savedText = nextText;
    if (m_text != nextText) {
        m_text = nextText;
        emit textChanged();
    }
    setEditable(editable);
    setError(editable
                 ? QString()
                 : QStringLiteral(
                     "This Note contains formatted Blocks and is read-only "
                     "in the plain-text editor."));
    return true;
}

void NoteBuffer::reset(const QString& error)
{
    m_saveTimer.stop();
    m_itemId = 0;
    m_dirty = false;
    m_savedText.clear();
    if (!m_text.isEmpty()) {
        m_text.clear();
        emit textChanged();
    }
    setEditable(false);
    setError(error);
}

void NoteBuffer::setText(const QString& text)
{
    if (!m_editable || m_itemId <= 0 || text == m_text) {
        return;
    }

    m_text = text;
    emit textChanged();
    if (text == m_savedText) {
        m_saveTimer.stop();
        m_dirty = false;
        setError({});
        return;
    }

    m_dirty = true;
    m_saveTimer.start();
}

bool NoteBuffer::flush()
{
    m_saveTimer.stop();
    if (!m_dirty || m_itemId <= 0) {
        return true;
    }

    BlockRepository blocks(m_database);
    if (!blocks.replaceText(m_itemId, m_text)) {
        setError(blocks.lastError());
        return false;
    }

    m_dirty = false;
    m_savedText = m_text;
    setError({});
    return true;
}
