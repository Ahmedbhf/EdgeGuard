#include "data_storage_service.h"

#include <algorithm>
#include <limits>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QTimeZone>

namespace {
constexpr qint64 RollingWindowMs = 24LL * 60LL * 60LL * 1000LL;
const char *HeaderLine = "timestamp,anomaly,x,y,z,temp\n";
}

void DataStorageService::appendSample(const SensorSample &sample)
{
    if (!ensureDatabase())
        return;

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
    if (!ensureDatabase())
        return;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM history_samples WHERE timestamp_ms < ?"));
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - RollingWindowMs);
    if (!query.exec())
        qWarning() << "Could not clean old SQLite history rows:" << query.lastError().text();
}

DataStorageService::HistoryChunk DataStorageService::loadLast24hSamples(int limit, int offset) const
{
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
