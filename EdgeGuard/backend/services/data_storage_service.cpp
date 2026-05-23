#include "data_storage_service.h"

#include <QDir>
#include <QSqlDatabase>
#include <QStandardPaths>

namespace {
// Chooses and creates the app-data location for the rolling SQLite database.
QString resolveDatabasePath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QDir::currentPath();

    QDir directory(basePath);
    directory.mkpath(".");
    return directory.filePath(QStringLiteral("rolling_24h_history.db"));
}
}

// Opens the history database and removes records outside the rolling window.
DataStorageService::DataStorageService()
    : m_storagePath(resolveDatabasePath())
    , m_connectionName(QStringLiteral("edgeguard_history_%1").arg(reinterpret_cast<quintptr>(this), 0, 16))
{
    ensureDatabase();
    cleanOldData();
}

// Closes and unregisters this service's private Qt SQL connection.
DataStorageService::~DataStorageService()
{
    if (!QSqlDatabase::contains(m_connectionName))
        return;

    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid() && db.isOpen())
            db.close();
    }

    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}
