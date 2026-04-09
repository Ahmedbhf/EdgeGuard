#include "data_storage_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>
#include <limits>

namespace {
constexpr qint64 RollingWindowMs = 24LL * 60LL * 60LL * 1000LL;
const char *HeaderLine = "timestamp,anomaly,x,y,z,temp\n";

QString resolveDatabasePath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QDir::currentPath();

    QDir directory(basePath);
    directory.mkpath(".");
    return directory.filePath(QStringLiteral("rolling_24h_history.db"));
}

QString resolveLegacyCsvPath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QDir::currentPath();

    QDir directory(basePath);
    directory.mkpath(".");
    return directory.filePath(QStringLiteral("rolling_24h_history.csv"));
}

QDateTime parseTimestampUtc(const QString &field)
{
    const QString trimmed = field.trimmed();
    QDateTime timestamp = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!timestamp.isValid())
        timestamp = QDateTime::fromString(trimmed, Qt::ISODate);
    return timestamp.isValid() ? timestamp.toUTC() : QDateTime();
}
}

DataStorageService::DataStorageService()
    : m_storagePath(resolveDatabasePath())
    , m_fallbackCsvPath(resolveLegacyCsvPath())
    , m_connectionName(QStringLiteral("edgeguard_history_%1").arg(reinterpret_cast<quintptr>(this), 0, 16))
{
    ensureDatabase();
    migrateLegacyCsvIfNeeded();
    cleanOldData();
}

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

void DataStorageService::appendSample(const SensorSample &sample)
{
    if (!ensureDatabase()) {
        appendSampleFallback(sample);
        return;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO history_samples (timestamp_ms, anomaly, x, y, z, temp) "
        "VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(sample.timestampUtc.toUTC().toMSecsSinceEpoch());
    query.addBindValue(sample.anomalyScore);
    query.addBindValue(sample.x);
    query.addBindValue(sample.y);
    query.addBindValue(sample.z);
    query.addBindValue(sample.temp);
    if (!query.exec())
        qWarning() << "Could not insert history sample into SQLite:" << query.lastError().text();
}

void DataStorageService::cleanOldData()
{
    if (!ensureDatabase()) {
        cleanFallbackCsv();
        return;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM history_samples WHERE timestamp_ms < ?"));
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - RollingWindowMs);
    if (!query.exec())
        qWarning() << "Could not clean old SQLite history rows:" << query.lastError().text();
}

DataStorageService::HistoryChunk DataStorageService::loadLast24hSamples(int limit, int offset) const
{
    if (!ensureDatabase())
        return loadFallbackCsvSamples(limit, offset);

    return queryLast24hSamples(limit, offset);
}

bool DataStorageService::exportCsv(const QString &destinationPath) const
{
    const QString targetPath = destinationPath.trimmed();
    if (targetPath.isEmpty())
        return false;

    const HistoryChunk chunk = loadLast24hSamples(std::numeric_limits<int>::max(), 0);
    if (chunk.samples.isEmpty())
        return false;

    QFileInfo targetInfo(targetPath);
    QDir().mkpath(targetInfo.absolutePath());

    QSaveFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream << HeaderLine;
    for (const SensorSample &sample : chunk.samples) {
        stream << sample.timestampUtc.toUTC().toString(Qt::ISODateWithMs)
               << ','
               << QString::number(sample.anomalyScore, 'f', 3)
               << ','
               << QString::number(sample.x, 'f', 4)
               << ','
               << QString::number(sample.y, 'f', 4)
               << ','
               << QString::number(sample.z, 'f', 4)
               << ','
               << QString::number(sample.temp, 'f', 2)
               << '\n';
    }
    return file.commit();
}

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

    if (!m_database.isOpen() && !m_database.open())
    {
        qWarning() << "Could not open SQLite history database:" << m_database.lastError().text()
                   << "path:" << m_storagePath;
        return false;
    }

    return ensureSchema();
}

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

void DataStorageService::migrateLegacyCsvIfNeeded() const
{
    if (!ensureDatabase())
        return;

    QSqlQuery countQuery(m_database);
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM history_samples")) || !countQuery.next())
        return;

    if (countQuery.value(0).toLongLong() > 0)
        return;

    QFile legacyFile(resolveLegacyCsvPath());
    if (!legacyFile.exists() || !legacyFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream stream(&legacyFile);
    bool firstLine = true;
    m_database.transaction();

    QSqlQuery insertQuery(m_database);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO history_samples (timestamp_ms, anomaly, x, y, z, temp) "
        "VALUES (?, ?, ?, ?, ?, ?)"));

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        if (firstLine) {
            firstLine = false;
            if (line.startsWith(QStringLiteral("timestamp,")))
                continue;
        }

        const QStringList fields = line.split(',');
        if (fields.size() < 6)
            continue;

        bool anomalyOk = false;
        bool xOk = false;
        bool yOk = false;
        bool zOk = false;
        bool tempOk = false;

        const QDateTime timestampUtc = parseTimestampUtc(fields[0]);
        const double anomaly = fields[1].trimmed().toDouble(&anomalyOk);
        const double x = fields[2].trimmed().toDouble(&xOk);
        const double y = fields[3].trimmed().toDouble(&yOk);
        const double z = fields[4].trimmed().toDouble(&zOk);
        const double temp = fields[5].trimmed().toDouble(&tempOk);

        if (!timestampUtc.isValid() || !anomalyOk || !xOk || !yOk || !zOk || !tempOk)
            continue;

        insertQuery.bindValue(0, timestampUtc.toMSecsSinceEpoch());
        insertQuery.bindValue(1, anomaly);
        insertQuery.bindValue(2, x);
        insertQuery.bindValue(3, y);
        insertQuery.bindValue(4, z);
        insertQuery.bindValue(5, temp);
        insertQuery.exec();
    }

    m_database.commit();
}

DataStorageService::HistoryChunk DataStorageService::queryLast24hSamples(int limit, int offset) const
{
    HistoryChunk chunk;
    if (!ensureDatabase())
        return chunk;

    const int safeLimit = std::max(1, limit);
    const int safeOffset = std::max(0, offset);
    const qint64 cutoffMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - RollingWindowMs;

    QSqlQuery countQuery(m_database);
    countQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM history_samples WHERE timestamp_ms >= ?"));
    countQuery.addBindValue(cutoffMs);
    if (countQuery.exec() && countQuery.next())
        chunk.totalCount = countQuery.value(0).toInt();

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT timestamp_ms, anomaly, x, y, z, temp "
        "FROM history_samples "
        "WHERE timestamp_ms >= ? "
        "ORDER BY timestamp_ms DESC "
        "LIMIT ? OFFSET ?"));
    query.addBindValue(cutoffMs);
    query.addBindValue(safeLimit);
    query.addBindValue(safeOffset);
    if (!query.exec()) {
        qWarning() << "Could not query SQLite history samples:" << query.lastError().text();
        return chunk;
    }

    while (query.next()) {
        SensorSample sample;
        sample.timestampUtc = QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong(), QTimeZone::UTC);
        sample.anomalyScore = query.value(1).toDouble();
        sample.x = query.value(2).toDouble();
        sample.y = query.value(3).toDouble();
        sample.z = query.value(4).toDouble();
        sample.temp = query.value(5).toDouble();
        chunk.samples.prepend(sample);
    }

    chunk.offset = safeOffset;
    return chunk;
}

void DataStorageService::ensureFallbackCsv() const
{
    QFileInfo fileInfo(m_fallbackCsvPath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(m_fallbackCsvPath);
    if (file.exists() && file.size() > 0)
        return;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    file.write(HeaderLine);
}

void DataStorageService::appendSampleFallback(const SensorSample &sample) const
{
    ensureFallbackCsv();

    QFile file(m_fallbackCsvPath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << sample.timestampUtc.toUTC().toString(Qt::ISODateWithMs)
           << ','
           << QString::number(sample.anomalyScore, 'f', 3)
           << ','
           << QString::number(sample.x, 'f', 4)
           << ','
           << QString::number(sample.y, 'f', 4)
           << ','
           << QString::number(sample.z, 'f', 4)
           << ','
           << QString::number(sample.temp, 'f', 2)
           << '\n';
}

DataStorageService::HistoryChunk DataStorageService::loadFallbackCsvSamples(int limit, int offset) const
{
    ensureFallbackCsv();

    HistoryChunk chunk;
    QFile file(m_fallbackCsvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return chunk;

    QTextStream stream(&file);
    const qint64 cutoffMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - RollingWindowMs;
    bool firstLine = true;
    QVector<SensorSample> allSamples;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        if (firstLine) {
            firstLine = false;
            if (line.startsWith(QStringLiteral("timestamp,")))
                continue;
        }

        const QStringList fields = line.split(',');
        if (fields.size() < 6)
            continue;

        bool anomalyOk = false;
        bool xOk = false;
        bool yOk = false;
        bool zOk = false;
        bool tempOk = false;

        SensorSample sample;
        sample.timestampUtc = parseTimestampUtc(fields[0]);
        sample.anomalyScore = fields[1].trimmed().toDouble(&anomalyOk);
        sample.x = fields[2].trimmed().toDouble(&xOk);
        sample.y = fields[3].trimmed().toDouble(&yOk);
        sample.z = fields[4].trimmed().toDouble(&zOk);
        sample.temp = fields[5].trimmed().toDouble(&tempOk);

        if (!sample.timestampUtc.isValid() || !anomalyOk || !xOk || !yOk || !zOk || !tempOk)
            continue;

        if (sample.timestampUtc.toMSecsSinceEpoch() < cutoffMs)
            continue;

        allSamples.append(sample);
    }

    const int totalSamples = static_cast<int>(allSamples.size());
    chunk.totalCount = totalSamples;

    const int safeLimit = std::max(1, limit);
    const int safeOffset = std::max(0, offset);
    if (safeOffset >= totalSamples) {
        chunk.offset = safeOffset;
        return chunk;
    }

    const int firstIndex = std::max(0, totalSamples - safeOffset - safeLimit);
    const int count = std::min(safeLimit, totalSamples - safeOffset - firstIndex);
    for (int i = firstIndex; i < firstIndex + count; ++i)
        chunk.samples.append(allSamples.at(i));

    chunk.offset = safeOffset;
    return chunk;
}

void DataStorageService::cleanFallbackCsv() const
{
    const HistoryChunk chunk = loadFallbackCsvSamples(std::numeric_limits<int>::max(), 0);
    ensureFallbackCsv();

    QSaveFile file(m_fallbackCsvPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << HeaderLine;
    for (const SensorSample &sample : chunk.samples) {
        stream << sample.timestampUtc.toUTC().toString(Qt::ISODateWithMs)
               << ','
               << QString::number(sample.anomalyScore, 'f', 3)
               << ','
               << QString::number(sample.x, 'f', 4)
               << ','
               << QString::number(sample.y, 'f', 4)
               << ','
               << QString::number(sample.z, 'f', 4)
               << ','
               << QString::number(sample.temp, 'f', 2)
               << '\n';
    }
    file.commit();
}
