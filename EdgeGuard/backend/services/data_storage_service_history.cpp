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
}

// Inserts one timestamped sensor sample into the rolling history table.
void DataStorageService::appendSample(const SensorSample &sample)
{
    if (!ensureDatabase())
        return;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO history_samples (device_id, timestamp_ms, anomaly, x, y, z, temp) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(sample.deviceId.trimmed());
    query.addBindValue(sample.timestampUtc.toUTC().toMSecsSinceEpoch());
    query.addBindValue(sample.anomalyScore);
    query.addBindValue(sample.x);
    query.addBindValue(sample.y);
    query.addBindValue(sample.z);
    query.addBindValue(sample.temp);
    if (!query.exec())
        qWarning() << "Could not insert history sample into SQLite:" << query.lastError().text();
}

// Deletes samples older than the configured 24-hour rolling window.
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

// Loads a paged slice of samples from the last 24 hours.
DataStorageService::HistoryChunk DataStorageService::loadLast24hSamples(int limit, int offset, const QString &deviceId) const
{
    return queryLast24hSamples(limit, offset, deviceId);
}

// Queries newest rows first, then restores chronological order for charting.
DataStorageService::HistoryChunk DataStorageService::queryLast24hSamples(int limit, int offset, const QString &deviceId) const
{
    HistoryChunk chunk;
    if (!ensureDatabase())
        return chunk;

    const int safeLimit = std::max(1, limit);
    const int safeOffset = std::max(0, offset);
    const qint64 cutoffMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - RollingWindowMs;
    const QString trimmedDeviceId = deviceId.trimmed();
    const bool filterByDevice = !trimmedDeviceId.isEmpty();

    QSqlQuery countQuery(m_database);
    countQuery.prepare(filterByDevice
                           ? QStringLiteral("SELECT COUNT(*) FROM history_samples WHERE device_id = ? AND timestamp_ms >= ?")
                           : QStringLiteral("SELECT COUNT(*) FROM history_samples WHERE timestamp_ms >= ?"));
    if (filterByDevice)
        countQuery.addBindValue(trimmedDeviceId);
    countQuery.addBindValue(cutoffMs);
    if (countQuery.exec() && countQuery.next())
        chunk.totalCount = countQuery.value(0).toInt();

    QSqlQuery query(m_database);
    query.prepare(filterByDevice
                      ? QStringLiteral(
                            "SELECT device_id, timestamp_ms, anomaly, x, y, z, temp "
                            "FROM history_samples "
                            "WHERE device_id = ? AND timestamp_ms >= ? "
                            "ORDER BY timestamp_ms DESC "
                            "LIMIT ? OFFSET ?")
                      : QStringLiteral(
                            "SELECT device_id, timestamp_ms, anomaly, x, y, z, temp "
                            "FROM history_samples "
                            "WHERE timestamp_ms >= ? "
                            "ORDER BY timestamp_ms DESC "
                            "LIMIT ? OFFSET ?"));
    if (filterByDevice)
        query.addBindValue(trimmedDeviceId);
    query.addBindValue(cutoffMs);
    query.addBindValue(safeLimit);
    query.addBindValue(safeOffset);
    if (!query.exec()) {
        qWarning() << "Could not query SQLite history samples:" << query.lastError().text();
        return chunk;
    }

    while (query.next()) {
        SensorSample sample;
        sample.deviceId = query.value(0).toString();
        sample.timestampUtc = QDateTime::fromMSecsSinceEpoch(query.value(1).toLongLong(), QTimeZone::UTC);
        sample.anomalyScore = query.value(2).toDouble();
        sample.x = query.value(3).toDouble();
        sample.y = query.value(4).toDouble();
        sample.z = query.value(5).toDouble();
        sample.temp = query.value(6).toDouble();
        chunk.samples.prepend(sample);
    }

    chunk.offset = safeOffset;
    return chunk;
}
