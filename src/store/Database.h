#ifndef OPENWODAY_STORE_DATABASE_H
#define OPENWODAY_STORE_DATABASE_H

#include <QSqlDatabase>
#include <QString>

// Database owns the single local SQLite connection. On open() it creates the
// app-data directory and the one database file, enables foreign-key
// enforcement, and applies the versioned migrations from the single versioned
// SQL source (schema.sql), writing the applied version to the meta table.
// Startup must fail loudly (non-zero exit, clear diagnostic) if open() fails,
// so a half-working planner is never left behind.
class Database
{
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Name of the single local SQLite file.
    static QString databaseFileName();
    // Platform app-data directory used by default (e.g.
    // $XDG_DATA_HOME/open-woday on Linux).
    static QString defaultDataDirectory();

    // Create/open the database and apply any pending migrations. Returns false
    // and sets lastError() on failure. Safe to call again after close().
    bool open();
    // Close the connection (idempotent).
    void close();

    // The schema version (0 before any migration has been applied).
    int schemaVersion() const;
    // Clear, human-readable diagnostic for a failed open(); empty on success.
    QString lastError() const;

private:
    bool applyMigrations(int currentVersion, int* outVersion);
    int readSchemaVersion(bool* ok);
    bool writeSchemaVersion(int version);
    bool execScript(const QString& sql);
    void fail(const QString& reason);
    void releaseConnection();

    QSqlDatabase m_db;
    QString m_error;
    int m_schemaVersion = 0;
};

#endif // OPENWODAY_STORE_DATABASE_H
