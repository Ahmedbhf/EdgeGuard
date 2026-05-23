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

// History storage/navigation: load chunks, export CSV, and store downsampled live samples.
// Resets paging to the newest history chunk and reloads chart data.
void AppController::refreshHistoryData()
{
    m_historyChunkOffset = 0;
    loadHistoryChunk();
}

// Moves the history view one chunk older if older samples are available.
void AppController::loadOlderHistoryChunk()
{
    const int totalCount = m_historyData.value(QStringLiteral("totalCount")).toInt();
    if (m_historyChunkOffset + HistoryChunkSize >= totalCount)
        return;

    m_historyChunkOffset += HistoryChunkSize;
    loadHistoryChunk();
}

// Moves the history view one chunk newer if the user is not at the latest data.
void AppController::loadNewerHistoryChunk()
{
    if (m_historyChunkOffset <= 0)
        return;

    m_historyChunkOffset = std::max(0, m_historyChunkOffset - HistoryChunkSize);
    loadHistoryChunk();
}

// Exports all locally stored 24-hour history samples to a CSV file.
bool AppController::exportHistoryCsv(const QUrl &fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    const QString targetPath = path.trimmed();
    if (targetPath.isEmpty()) {
        appendLog(QStringLiteral("Could not export the 24h history CSV."));
        return false;
    }

    const DataStorageService::HistoryChunk chunk = m_storageService.loadLast24hSamples(std::numeric_limits<int>::max(), 0);
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

// Downsamples live aggregated values into the rolling SQLite history store.
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
    m_storageService.appendSample(sample);

    m_lastStoredSampleMs = nowMs;
    ++m_storageSamplesSinceCleanup;
    if (m_storageSamplesSinceCleanup >= StorageCleanupIntervalSamples) {
        m_storageService.cleanOldData();
        m_storageSamplesSinceCleanup = 0;
    }
}

// Loads one paged history slice and attaches pagination metadata for QML.
void AppController::loadHistoryChunk()
{
    const DataStorageService::HistoryChunk chunk = m_storageService.loadLast24hSamples(HistoryChunkSize, m_historyChunkOffset);
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
    m_historyStatusText = totalCount > loadedCount
                              ? QStringLiteral("Showing samples %1-%2 of %3 from the local 24-hour history database.")
                                    .arg(newestStart)
                                    .arg(newestEnd)
                                    .arg(totalCount)
                              : QStringLiteral("%1 samples loaded from the local 24-hour history database.")
                                    .arg(loadedCount);
    emit historyDataChanged();
}

// Applies parsed chart data or clears history when parsing found no samples.
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

// Clears history charts and publishes a status message explaining why.
void AppController::clearHistoryData(const QString &statusText)
{
    m_historyData.clear();
    m_historyStatusText = statusText;
    emit historyDataChanged();
}
