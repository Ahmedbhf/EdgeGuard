#include "app_controller.h"

#include <QDateTime>
#include <QSaveFile>
#include <QTextStream>
#include <QTimeZone>

#include <limits>

namespace {
// Formats a number with fixed decimal precision for CSV output.
QString formatNumber(double value, int precision)
{
    return QString::number(value, 'f', precision);
}

// Escapes one CSV cell, quoting it only when the value needs protection.
QString csvCell(QString value)
{
    value.replace('"', QStringLiteral("\"\""));
    if (value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r'))
        return QStringLiteral("\"%1\"").arg(value);
    return value;
}

// Writes one comma-separated row to the export stream.
void writeCsvRow(QTextStream &stream, const QStringList &columns)
{
    stream << columns.join(',') << '\n';
}
}

// =========================================================================
// 1. refreshHistoryData (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Refresh" ControlButton in HistoryPage.qml (Line 75)
//               and automatically upon component instantiation completion (Line 50 of HistoryPage.qml)
// Role: Resets the SQLite pagination offset `m_historyChunkOffset` to 0 (meaning the newest entries)
//       and queries the SQLite database via `loadHistoryChunk()` to populate history charts.
void AppController::refreshHistoryData()
{
    m_historyChunkOffset = 0;
    loadHistoryChunk();
}

// =========================================================================
// 2. loadOlderHistoryChunk (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Older" ControlButton in HistoryPage.qml (Line 81)
// Role: Increments `m_historyChunkOffset` by 1000 (HistoryChunkSize) to shift the database window,
//       fetching and displaying the next older page of 1000 historical samples from SQLite.
void AppController::loadOlderHistoryChunk()
{
    const int totalCount = m_historyData.value(QStringLiteral("totalCount")).toInt();
    if (m_historyChunkOffset + HistoryChunkSize >= totalCount)
        return;

    m_historyChunkOffset += HistoryChunkSize;
    loadHistoryChunk();
}

// =========================================================================
// 3. loadNewerHistoryChunk (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Newer" ControlButton in HistoryPage.qml (Line 87)
// Role: Decrements `m_historyChunkOffset` by 1000 (HistoryChunkSize) to slide the database window
//       forward in time, fetching and displaying a newer page of 1000 historical samples.
void AppController::loadNewerHistoryChunk()
{
    if (m_historyChunkOffset <= 0)
        return;

    m_historyChunkOffset = std::max(0, m_historyChunkOffset - HistoryChunkSize);
    loadHistoryChunk();
}

// =========================================================================
// 4. exportHistoryCsv (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Export CSV" ControlButton in HistoryPage.qml (Line 93)
//               via the accepted callback on FileDialog (Line 58)
// Role: Loads the complete 24-hour database records for the current device and formats them
//       into a standard comma-separated text file (CSV), then writes it to the local disk.
bool AppController::exportHistoryCsv(const QUrl &fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    const QString targetPath = path.trimmed();
    if (targetPath.isEmpty()) {
        appendLog(QStringLiteral("Could not export the 24h history CSV."));
        return false;
    }

    const QString deviceId = m_deviceId.trimmed();
    const DataStorageService::HistoryChunk chunk =
        m_storageService.loadLast24hSamples(std::numeric_limits<int>::max(), 0, deviceId);
    if (chunk.samples.isEmpty()) {
        appendLog(QStringLiteral("Could not export the 24h history CSV."));
        return false;
    }

    QSaveFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog(QStringLiteral("Could not export the 24h history CSV."));
        return false;
    }

    QTextStream stream(&file);
    writeCsvRow(stream, {
                            QStringLiteral("timestamp_utc"),
                            QStringLiteral("timestamp_ms"),
                            QStringLiteral("device_id"),
                            QStringLiteral("anomaly_score"),
                            QStringLiteral("condition"),
                            QStringLiteral("x"),
                            QStringLiteral("y"),
                            QStringLiteral("z"),
                            QStringLiteral("temp_c")
                        });

    for (const SensorSample &sample : chunk.samples) {
        writeCsvRow(stream, {
                                csvCell(sample.timestampUtc.toUTC().toString(Qt::ISODateWithMs)),
                                QString::number(sample.timestampUtc.toUTC().toMSecsSinceEpoch()),
                                csvCell(sample.deviceId),
                                formatNumber(sample.anomalyScore, 3),
                                csvCell(SensorSample::stateForScore(sample.anomalyScore)),
                                formatNumber(sample.x, 4),
                                formatNumber(sample.y, 4),
                                formatNumber(sample.z, 4),
                                formatNumber(sample.temp, 2)
                            });
    }

    const bool ok = file.commit();
    appendLog(ok
                  ? QStringLiteral("Exported 24h dataset to %1").arg(path)
                  : QStringLiteral("Could not export the 24h history CSV."));
    return ok;
}

// =========================================================================
// 5. storeHistorySample (Internal Helper)
// =========================================================================
// Triggered by: AppController::flushLiveData() in app_controller_live.cpp (Line 60)
// Role: Saves averaged sensor metrics down to the local SQLite file every 250ms.
//       Also cleanup old logs (retaining only the last 24 hours) every 20 samples.
void AppController::storeHistorySample(double anomalyScore, double x, double y, double z, double temp)
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs - m_lastStoredSampleMs < StorageIntervalMs)
        return;

    SensorSample sample;
    sample.timestampUtc = QDateTime::fromMSecsSinceEpoch(nowMs, QTimeZone::UTC);
    sample.anomalyScore = anomalyScore;
    sample.x = x;
    sample.y = y;
    sample.z = z;
    sample.temp = temp;
    sample.deviceId = m_latestSample.deviceId.trimmed().isEmpty()
                          ? m_deviceId.trimmed()
                          : m_latestSample.deviceId.trimmed();
    m_storageService.appendSample(sample);

    m_lastStoredSampleMs = nowMs;
    ++m_storageSamplesSinceCleanup;
    if (m_storageSamplesSinceCleanup >= StorageCleanupIntervalSamples) {
        m_storageService.cleanOldData();
        m_storageSamplesSinceCleanup = 0;
    }
}

// =========================================================================
// 6. loadHistoryChunk (Internal Helper)
// =========================================================================
// Triggered internally by: refreshHistoryData(), loadOlderHistoryChunk(), loadNewerHistoryChunk()
// Role: Queries the DataStorageService for a slice of size 1000 from the SQLite database
//       based on the active offset. It converts them to chart-ready objects, sets boundary properties
//       for navigation indicators, and formats the descriptive page indexing range text.
void AppController::loadHistoryChunk()
{
    const QString deviceId = m_deviceId.trimmed();
    const DataStorageService::HistoryChunk chunk =
        m_storageService.loadLast24hSamples(HistoryChunkSize, m_historyChunkOffset, deviceId);
    updateHistoryData(parseHistorySamples(chunk.samples));

    if (m_historyData.isEmpty())
        return;

    const int loadedCount = chunk.samples.size();
    const int totalCount = chunk.totalCount;
    const int newestStart = totalCount > 0 ? std::max(1, totalCount - chunk.offset - loadedCount + 1) : 0;
    const int newestEnd = totalCount > 0 ? (totalCount - chunk.offset) : 0;

    m_historyData.insert(QStringLiteral("loadedCount"), loadedCount);
    m_historyData.insert(QStringLiteral("totalCount"), totalCount);
    m_historyData.insert(QStringLiteral("hasOlder"), chunk.offset + loadedCount < totalCount);
    m_historyData.insert(QStringLiteral("hasNewer"), chunk.offset > 0);
    m_historyData.insert(QStringLiteral("chunkStartIndex"), newestStart);
    m_historyData.insert(QStringLiteral("chunkEndIndex"), newestEnd);
    m_historyData.insert(QStringLiteral("deviceId"), deviceId);

    const QString historyScope = deviceId.isEmpty()
                                     ? QStringLiteral("the local 24-hour history database")
                                     : QStringLiteral("the local 24-hour history database for device %1").arg(deviceId);
    m_historyStatusText = totalCount > loadedCount
                              ? QStringLiteral("Showing samples %1-%2 of %3 from %4.")
                                    .arg(newestStart)
                                    .arg(newestEnd)
                                    .arg(totalCount)
                                    .arg(historyScope)
                              : QStringLiteral("%1 samples loaded from %2.")
                                    .arg(loadedCount)
                                    .arg(historyScope);
    emit historyDataChanged();
}

// =========================================================================
// 7. updateHistoryData (Internal Helper)
// =========================================================================
// Triggered internally by: loadHistoryChunk()
// Role: Updates the exposed properties with the newly parsed structures and triggers
//       `historyDataChanged()` to notify QML graph elements (HistoryChart, FftSpectrumChart) to redraw.
void AppController::updateHistoryData(const ParsedHistory &parsedHistory)
{
    if (parsedHistory.data.isEmpty()) {
        clearHistoryData(parsedHistory.statusText);
        return;
    }

    m_historyData = parsedHistory.data;
    m_historyStatusText = parsedHistory.statusText;
    emit historyDataChanged();
}

// =========================================================================
// 8. clearHistoryData (Internal Helper)
// =========================================================================
// Triggered internally by: updateHistoryData()
// Role: Clears the historical data cache map when no records exist or query returns empty,
//       publishing an informative status string to the UI.
void AppController::clearHistoryData(const QString &statusText)
{
    m_historyData.clear();
    m_historyStatusText = statusText;
    emit historyDataChanged();
}
