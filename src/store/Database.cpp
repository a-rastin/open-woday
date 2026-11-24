#include "Database.h"

#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

#include <algorithm>

namespace {

const char* const kConnectionName = "open-woday-store";

// Versioned migrations. Every migration maps a monotonically increasing version
// to the SQL source that upgrades the database to it. v0.1 has a single
// migration that creates the complete schema; the migration list itself is the
// only place versions are declared, and the SQL for every version lives in
// schema.sql.
struct Migration {
    int version;
    const char* resource;
};

const Migration kMigrations[] = {
    { 1, ":/store/schema.sql" },
};

// Split a trusted SQL script into single statements. Qt's SQLite driver refuses
// to execute more than one statement per QSqlQuery::exec, so the versioned
// source is applied statement by statement. The schema source is trusted v0.1
// DDL with no semicolons inside quoted strings or comments, so a plain split
// on ';' is all that is needed (tech-stack.md §7.1: simple versioned SQL).
QStringList splitSqlStatements(const QString& script)
{
    QStringList statements;
    const QStringList chunks = script.split(QLatin1Char(';'));
    for (const QString& chunk : chunks) {
        const QString statement = chunk.trimmed();
        if (!statement.isEmpty()) {
            statements.append(statement);
        }
    }
    return statements;
}

int latestMigrationVersion()
{
    int latest = 0;
    for (const Migration& migration : kMigrations) {
        latest = std::max(latest, migration.version);
    }
    return latest;
}

} // namespace

Database::Database() = default;

Database::~Database()
{
    close();
}

QString Database::databaseFileName()
{
    return QStringLiteral("open-woday.db");
}

QString Database::defaultDataDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

bool Database::open()
{
    m_error.clear();
    m_schemaVersion = 0;

    const QString directory = defaultDataDirectory();
    QDir dir;
    if (!dir.mkpath(directory)) {
        fail(QStringLiteral("could not create the app-data directory '%1'")
                 .arg(directory));
        return false;
    }

    const QString databasePath =
        directory + QLatin1Char('/') + databaseFileName();

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                     QString::fromUtf8(kConnectionName));
    m_db.setDatabaseName(databasePath);
    if (!m_db.open()) {
        fail(QStringLiteral("could not open the SQLite file '%1': %2")
                 .arg(databasePath, m_db.lastError().text()));
        releaseConnection();
        return false;
    }

    // Foreign-key enforcement must be enabled before any migration runs and
    // outside of a transaction. Every write below uses bound values; no SQL is
    // assembled by concatenating user text.
    QSqlQuery pragma(m_db);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        fail(QStringLiteral("could not enable SQLite foreign-key enforcement: %1")
                 .arg(pragma.lastError().text()));
        releaseConnection();
        return false;
    }

    bool versionRead = false;
    const int current = readSchemaVersion(&versionRead);
    if (!versionRead) {
        releaseConnection();
        return false;
    }

    int applied = current;
    if (!applyMigrations(current, &applied)) {
        releaseConnection();
        return false;
    }
    if (applied != latestMigrationVersion()) {
        fail(QStringLiteral("the database schema version (%1) is newer than the "
                            "schema this build understands (%2)")
                 .arg(applied).arg(latestMigrationVersion()));
        releaseConnection();
        return false;
    }

    m_schemaVersion = applied;
    return true;
}

void Database::close()
{
    releaseConnection();
}

int Database::schemaVersion() const
{
    return m_schemaVersion;
}

QString Database::lastError() const
{
    return m_error;
}

QSqlDatabase Database::connection() const
{
    return m_db;
}

int Database::readSchemaVersion(bool* ok)
{
    *ok = false;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?"));
    query.addBindValue(QStringLiteral("meta"));
    if (!query.exec()) {
        fail(QStringLiteral("could not inspect the database schema: %1")
                 .arg(query.lastError().text()));
        return 0;
    }
    if (!query.next()) {
        // A fresh database has no meta table yet, so no migration has run.
        *ok = true;
        return 0;
    }

    query.prepare(QStringLiteral("SELECT value FROM meta WHERE key = ?"));
    query.addBindValue(QStringLiteral("schema_version"));
    if (!query.exec()) {
        fail(QStringLiteral("could not read the schema version from meta: %1")
                 .arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        *ok = true;
        return query.value(0).toInt();
    }

    // meta exists but has no version row yet; treat as version 0.
    *ok = true;
    return 0;
}

bool Database::writeSchemaVersion(int version)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO meta (key, value) VALUES (?, ?)"));
    query.addBindValue(QStringLiteral("schema_version"));
    query.addBindValue(version);
    if (!query.exec()) {
        fail(QStringLiteral("could not write schema version %1 to meta: %2")
                 .arg(version).arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool Database::execScript(const QString& sql)
{
    const QStringList statements = splitSqlStatements(sql);
    if (statements.isEmpty()) {
        fail(QStringLiteral("the versioned schema source contains no SQL"));
        return false;
    }
    for (const QString& statement : statements) {
        QSqlQuery query(m_db);
        if (!query.exec(statement)) {
            fail(QStringLiteral("could not apply the versioned schema: %1")
                     .arg(query.lastError().text()));
            return false;
        }
    }
    return true;
}

bool Database::applyMigrations(int currentVersion, int* outVersion)
{
    QList<int> pending;
    for (const Migration& migration : kMigrations) {
        if (migration.version > currentVersion) {
            pending.append(migration.version);
        }
    }
    std::sort(pending.begin(), pending.end());

    for (int version : pending) {
        QString resource;
        for (const Migration& migration : kMigrations) {
            if (migration.version == version) {
                resource = QString::fromUtf8(migration.resource);
                break;
            }
        }

        QFile file(resource);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fail(QStringLiteral("migration %1 SQL source '%2' could not be read")
                     .arg(version).arg(resource));
            return false;
        }
        const QString sql = QString::fromUtf8(file.readAll());

        // Each migration is atomic: the DDL and the version value commit
        // together. No migration drops or rebuilds a table holding user data.
        if (!m_db.transaction()) {
            fail(QStringLiteral("could not begin migration %1: %2")
                     .arg(version).arg(m_db.lastError().text()));
            return false;
        }
        if (!execScript(sql)) {
            m_db.rollback();
            return false;
        }
        if (!writeSchemaVersion(version)) {
            m_db.rollback();
            return false;
        }
        if (!m_db.commit()) {
            fail(QStringLiteral("could not commit migration %1: %2")
                     .arg(version).arg(m_db.lastError().text()));
            return false;
        }
        *outVersion = version;
    }

    if (pending.isEmpty()) {
        *outVersion = currentVersion;
    }
    return true;
}

void Database::fail(const QString& reason)
{
    m_error = reason;
}

void Database::releaseConnection()
{
    if (m_db.isValid()) {
        if (m_db.isOpen()) {
            m_db.close();
        }
        const QString name = m_db.connectionName();
        m_db = QSqlDatabase();          // drop our handle before removing
        QSqlDatabase::removeDatabase(name);
    }
}
