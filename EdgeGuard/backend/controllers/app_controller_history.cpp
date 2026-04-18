#include "app_controller.h"

#include <QDateTime>
#include <QTimeZone>

#include <algorithm>
#include <limits>

// History storage/navigation: load chunks, export CSV, and store downsampled live samples.
void AppController::refreshHistoryData()
{
    m_historyChunkOffset = 0;
    loadHistoryChunk();
}

void AppController::loadOlderHistoryChunk()
{
    const int totalCount = m_historyData.value(QStringLiteral("totalCount")).toInt();
    if (m_historyChunkOffset + HistoryChunkSize >= totalCount)
        return;

    m_historyChunkOffset += HistoryChunkSize;
    loadHistoryChunk();
}

void AppController::loadNewerHistoryChunk()
{
    if (m_historyChunkOffset <= 0)
        return;

    m_historyChunkOffset = std::max(0, m_historyChunkOffset - HistoryChunkSize);
    loadHistoryChunk();
}

bool AppController::exportHistoryCsv(const QUrl &fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    const bool ok = m_storageService.exportCsv(path);
    appendLog(ok
                  ? QStringLiteral("Exported 24h history to %1").arg(path)
                  : QStringLiteral("Could not export the 24h history CSV."));
    return ok;
}

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

void AppController::clearHistoryData(const QString &statusText)
{
    m_historyData.clear();
    m_historyStatusText = statusText;
    emit historyDataChanged();
}
