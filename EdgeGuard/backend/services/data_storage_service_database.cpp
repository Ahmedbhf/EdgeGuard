#include "data_storage_service.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

// Opens the SQLite connection on demand and guarantees the schema exists.
bool DataStorageService::ensureDatabase() const
{
    QFileInfo fileInfo(m_storagePath);
    QDir().mkpath(fileInfo.absolutePath());

    if (!m_database.isValid()) {
        if (QSqlDatabase::contains(m_connectionName))
            m_database = QSqlDatabase::database(m_connectionName);
        else
            m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);

        m_database.setDatabaseName(m_storagePath);
    }

    if (!m_database.isOpen() && !m_database.open()) {
        qWarning() << "Could not open SQLite history database:" << m_database.lastError().text()
                   << "path:" << m_storagePath;
        return false;
    }

    return ensureSchema();
}

// Creates the history table and timestamp index once per service instance.
bool DataStorageService::ensureSchema() const
{
    if (m_schemaReady)
        return true;

    QSqlQuery pragmaQuery(m_database);
    pragmaQuery.exec(QStringLiteral("PRAGMA journal_mode = WAL"));

    QSqlQuery createTableQuery(m_database);
    if (!createTableQuery.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS history_samples ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "timestamp_ms INTEGER NOT NULL,"
            "anomaly REAL NOT NULL,"
            "x REAL NOT NULL,"
            "y REAL NOT NULL,"
            "z REAL NOT NULL,"
            "temp REAL NOT NULL)")))
    {
        qWarning() << "Could not create SQLite history table:" << createTableQuery.lastError().text();
        return false;
    }

    QSqlQuery createIndexQuery(m_database);
    if (!createIndexQuery.exec(
            QStringLiteral("CREATE INDEX IF NOT EXISTS idx_history_samples_timestamp ON history_samples(timestamp_ms)")))
    {
        qWarning() << "Could not create SQLite history index:" << createIndexQuery.lastError().text();
        return false;
    }

    m_schemaReady = true;
    return true;
}
