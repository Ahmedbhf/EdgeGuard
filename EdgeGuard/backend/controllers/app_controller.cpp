#include "app_controller.h"

#include <QDateTime>
#include <QHash>
#include <QTime>
#include <QTimeZone>

#include <algorithm>
#include <cmath>

namespace {
constexpr int HistoryFftWindowSize = 256;

QVariantMap computePaddedRange(const QVector<double> &values, bool clampMinToZero = true)
{
    if (values.isEmpty())
        return { { QStringLiteral("min"), 0.0 }, { QStringLiteral("max"), 1.0 } };

    double minValue = values.first();
    double maxValue = values.first();
    for (double value : values) {
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    if (qFuzzyCompare(minValue, maxValue)) {
        const double padding = std::max(1.0, std::abs(minValue) * 0.2);
        const double nextMin = minValue - padding;
        return {
            { QStringLiteral("min"), clampMinToZero ? std::max(0.0, nextMin) : nextMin },
            { QStringLiteral("max"), maxValue + padding }
        };
    }

    const double spread = maxValue - minValue;
    const double nextMin = minValue - spread * 0.12;
    return {
        { QStringLiteral("min"), clampMinToZero ? std::max(0.0, nextMin) : nextMin },
        { QStringLiteral("max"), maxValue + spread * 0.12 }
    };
}

QVariantList toVariantList(const QVector<double> &values)
{
    QVariantList list;
    list.reserve(values.size());
    for (double value : values)
        list.append(value);
    return list;
}

double estimateSampleRateHz(const QVector<qint64> &timestampsMs)
{
    if (timestampsMs.size() < 2)
        return 0.0;

    const qint64 durationMs = timestampsMs.last() - timestampsMs.first();
    if (durationMs <= 0)
        return 0.0;

    return ((timestampsMs.size() - 1) * 1000.0) / durationMs;
}
}

AppController::AppController(QObject *parent) : QObject(parent)
{
    connect(&m_serialService, &SerialService::deviceIdReceived, this, [this](const QString &deviceId) {
        const QString trimmedDeviceId = deviceId.trimmed();
        if (!trimmedDeviceId.isEmpty() && m_deviceId != trimmedDeviceId) {
            m_deviceId = trimmedDeviceId;
            emit deviceIdChanged();
        }

        if (m_machineType.isEmpty())
            setMachineType(QStringLiteral("DC Motor"));
    });
    connect(&m_serialService, &SerialService::sampleReceived, this, &AppController::onSampleReceived);
    connect(&m_serialService, &SerialService::errorOccurred, this, [this](const QString &text) { appendLog(text); });
    connect(&m_serialService, &SerialService::portsChanged, this, &AppController::syncPorts);
    connect(&m_serialService, &SerialService::connectedChanged, this, &AppController::syncConnection);

    m_liveDataTimer.setInterval(LiveAggregationIntervalMs);
    connect(&m_liveDataTimer, &QTimer::timeout, this, &AppController::flushLiveData);
    m_liveDataTimer.start();

    m_lastUpdateTimer.setInterval(1000);
    connect(&m_lastUpdateTimer, &QTimer::timeout, this, &AppController::updateLastUpdateText);
    m_lastUpdateTimer.start();

    m_lastStoredSampleMs = QDateTime::currentMSecsSinceEpoch();
    refreshPorts();
    refreshHistoryData();
    appendLog(QStringLiteral("Ready."));
}

QString AppController::lastUpdateText() const
{
    if (!m_latestSample.timestampUtc.isValid())
        return QStringLiteral("Waiting for data");

    const qint64 seconds = m_latestSample.timestampUtc.secsTo(QDateTime::currentDateTimeUtc());
    if (seconds <= 1)
        return QStringLiteral("Just now");

    return QStringLiteral("%1 s ago").arg(seconds);
}

void AppController::connectToPort(const QString &portName)
{
    const QString port = portName.split(' ').first().trimmed();
    if (port.isEmpty()) {
        appendLog(QStringLiteral("No serial device selected."));
        return;
    }

    setSelectedPort(port);
    if (m_serialService.connectToPort(port))
        appendLog(QStringLiteral("Connected to %1").arg(port));
}

void AppController::connectPreferredPort(const QString &preferredPort)
{
    refreshPorts();

    QString targetPort;
    for (int index = 0; index < availablePorts().size(); ++index) {
        if (portNameAt(index) == preferredPort.trimmed()) {
            targetPort = preferredPort.trimmed();
            break;
        }
    }

    if (targetPort.isEmpty() && !m_selectedPort.isEmpty())
        targetPort = m_selectedPort;
    if (targetPort.isEmpty() && !availablePorts().isEmpty())
        targetPort = portNameAt(0);
    if (targetPort.isEmpty())
        targetPort = preferredPort.trimmed();

    connectToPort(targetPort);
}

void AppController::disconnectPort()
{
    if (connected())
        m_serialService.disconnectPort();
}

void AppController::disconnectAndReset()
{
    disconnectPort();
    resetDeviceIdentity();
}

void AppController::setSelectedPort(const QString &portName)
{
    const QString port = portName.trimmed();
    if (m_selectedPort == port)
        return;

    m_selectedPort = port;
    emit selectedPortChanged();
}

void AppController::resetDeviceIdentity()
{
    bool deviceChanged = false;
    if (!m_deviceId.isEmpty()) {
        m_deviceId.clear();
        deviceChanged = true;
    }

    if (!m_machineType.isEmpty()) {
        m_machineType.clear();
        emit machineTypeChanged();
    }

    if (deviceChanged)
        emit deviceIdChanged();
}

void AppController::setMachineType(const QString &machineType)
{
    const QString trimmedMachineType = machineType.trimmed();
    if (m_machineType == trimmedMachineType)
        return;

    m_machineType = trimmedMachineType;
    emit machineTypeChanged();
}

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

void AppController::onSampleReceived(const SensorSample &sample)
{
    processSample(sample);
}

void AppController::flushLiveData()
{
    if (!m_pendingLiveRefresh && m_windowSampleCount == 0)
        return;

    if (m_windowSampleCount > 0) {
        const double aggregatedRms = std::sqrt(m_windowRmsSquareSum / m_windowSampleCount);
        const double aggregatedTemp = m_windowTempSum / m_windowSampleCount;
        const double aggregatedAnomaly = m_windowAnomalySum / m_windowSampleCount;
        const double aggregatedX = m_windowXSum / m_windowSampleCount;
        const double aggregatedY = m_windowYSum / m_windowSampleCount;
        const double aggregatedZ = m_windowZSum / m_windowSampleCount;

        appendValue(m_anomalyValues, aggregatedAnomaly, MaxHistory);
        appendValue(m_vibrationValues, aggregatedRms, MaxHistory);
        appendValue(m_temperatureValues, aggregatedTemp, MaxHistory);
        appendValue(m_xAxisValues, aggregatedX, MaxHistory);
        appendValue(m_yAxisValues, aggregatedY, MaxHistory);
        appendValue(m_zAxisValues, aggregatedZ, MaxHistory);

        emit anomalyValuesChanged();
        emit vibrationValuesChanged();
        emit temperatureValuesChanged();
        emit xAxisValuesChanged();
        emit yAxisValuesChanged();
        emit zAxisValuesChanged();

        updateFaultType();
        storeHistorySample(aggregatedAnomaly, aggregatedX, aggregatedY, aggregatedZ, aggregatedTemp);
    }

    m_windowSampleCount = 0;
    m_windowRmsSquareSum = 0.0;
    m_windowTempSum = 0.0;
    m_windowAnomalySum = 0.0;
    m_windowXSum = 0.0;
    m_windowYSum = 0.0;
    m_windowZSum = 0.0;

    if (m_pendingLiveRefresh)
        emit dataChanged();

    m_pendingLiveRefresh = false;
}

void AppController::updateLastUpdateText()
{
    emit lastUpdateTextChanged();
}

void AppController::processSample(const SensorSample &sample)
{
    const bool stateUpdated = sample.state != m_state;
    m_latestSample = sample;
    m_state = sample.state;
    m_anomalyScore = sample.anomalyScore;
    m_x = sample.x;
    m_y = sample.y;
    m_z = sample.z;
    m_temp = sample.temp;
    m_ambientTemp = sample.ambientTemp;
    updateLiveMetrics();

    m_windowRmsSquareSum += (m_rms * m_rms);
    m_windowTempSum += m_temp;
    m_windowAnomalySum += m_anomalyScore;
    m_windowXSum += m_x;
    m_windowYSum += m_y;
    m_windowZSum += m_z;
    ++m_windowSampleCount;
    m_pendingLiveRefresh = true;

    emit lastUpdateTextChanged();

    if (stateUpdated) {
        emit stateChanged();
        appendLog(QStringLiteral("State: %1").arg(m_state));
    }
}

void AppController::syncPorts()
{
    emit availablePortsChanged();

    QString nextPort = m_selectedPort;
    bool keepSelectedPort = false;
    for (int index = 0; index < availablePorts().size(); ++index)
        keepSelectedPort |= portNameAt(index) == m_selectedPort;

    if (!keepSelectedPort)
        nextPort = availablePorts().isEmpty() ? QString() : portNameAt(0);

    if (nextPort != m_selectedPort) {
        m_selectedPort = nextPort;
        emit selectedPortChanged();
    }
}

void AppController::syncConnection()
{
    if (!connected()) {
        flushLiveData();
        appendLog(QStringLiteral("Disconnected from %1").arg(m_selectedPort.isEmpty() ? QStringLiteral("device") : m_selectedPort));
    }

    emit connectedChanged();
}

void AppController::updateLiveMetrics()
{
    m_rms = m_latestSample.rms();
}

void AppController::updateFaultType()
{
    const LiveFaultFeatures features = computeLiveFaultFeatures();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    if (m_anomalyScore > 80.0) {
        setFaultDecision({
            QStringLiteral("NORMAL"),
            QStringLiteral("HIGH"),
            QStringLiteral("NanoEdge similarity indicates normal behavior")
        });
        m_faultPredictionHistory.clear();
        m_pendingFaultType.clear();
        m_pendingFaultSinceMs = 0;
        return;
    }

    if (!features.valid) {
        setFaultDecision({
            QStringLiteral("CALIBRATING"),
            QStringLiteral("LOW"),
            QStringLiteral("Waiting for enough live vibration data")
        });
        return;
    }

    const FaultDecision rawDecision = classifyFault(features.std,
                                                    features.peak,
                                                    features.dominantFreqHz);
    const FaultDecision smoothedDecision = smoothFaultDecision(rawDecision);

    if (m_faultType == smoothedDecision.type) {
        setFaultDecision(smoothedDecision);
        m_pendingFaultType.clear();
        m_pendingFaultSinceMs = 0;
        return;
    }

    if (m_pendingFaultType != smoothedDecision.type) {
        m_pendingFaultType = smoothedDecision.type;
        m_pendingFaultSinceMs = nowMs;
        return;
    }

    if (nowMs - m_pendingFaultSinceMs < FaultPromotionCooldownMs)
        return;

    setFaultDecision(smoothedDecision);
    appendLog(QStringLiteral("Fault type: %1 (%2)").arg(m_faultType, m_faultConfidence));
    m_pendingFaultType.clear();
    m_pendingFaultSinceMs = 0;
}

AppController::FaultDecision AppController::smoothFaultDecision(const FaultDecision &rawDecision)
{
    m_faultPredictionHistory.append(rawDecision);
    while (m_faultPredictionHistory.size() > FaultStabilizationWindow)
        m_faultPredictionHistory.remove(0);

    QHash<QString, int> counts;
    QString majorityType = rawDecision.type;
    int majorityCount = 0;

    for (const FaultDecision &decision : std::as_const(m_faultPredictionHistory)) {
        const int count = ++counts[decision.type];
        if (count > majorityCount || (count == majorityCount && decision.type == rawDecision.type)) {
            majorityCount = count;
            majorityType = decision.type;
        }
    }

    for (int index = m_faultPredictionHistory.size() - 1; index >= 0; --index) {
        if (m_faultPredictionHistory[index].type == majorityType)
            return m_faultPredictionHistory[index];
    }

    return rawDecision;
}

void AppController::setFaultDecision(const FaultDecision &decision)
{
    m_faultType = decision.type;
    m_faultConfidence = decision.confidence;
    m_faultReason = decision.reason;
    m_faultTypeTone = toneForFaultType(m_faultType);
}

AppController::LiveFaultFeatures AppController::computeLiveFaultFeatures() const
{
    LiveFaultFeatures features;

    int windowSize = std::min({ LiveFaultFftWindowSize,
                                static_cast<int>(m_xAxisValues.size()),
                                static_cast<int>(m_yAxisValues.size()),
                                static_cast<int>(m_zAxisValues.size()) });
    if (windowSize < 32)
        return features;

    int powerOfTwo = 1;
    while ((powerOfTwo << 1) <= windowSize)
        powerOfTwo <<= 1;
    windowSize = powerOfTwo;

    const int xStartIndex = m_xAxisValues.size() - windowSize;
    const int yStartIndex = m_yAxisValues.size() - windowSize;
    const int zStartIndex = m_zAxisValues.size() - windowSize;
    const QVector<double> xWindow(m_xAxisValues.begin() + xStartIndex, m_xAxisValues.end());
    const QVector<double> yWindow(m_yAxisValues.begin() + yStartIndex, m_yAxisValues.end());
    const QVector<double> zWindow(m_zAxisValues.begin() + zStartIndex, m_zAxisValues.end());

    QVector<double> combinedSignal;
    combinedSignal.reserve(windowSize);
    for (int index = 0; index < windowSize; ++index) {
        const double x = xWindow[index];
        const double y = yWindow[index];
        const double z = zWindow[index];
        combinedSignal.append(std::sqrt(((x * x) + (y * y) + (z * z)) / 3.0));
    }

    if (combinedSignal.size() < 32)
        return features;

    double sumSquares = 0.0;
    double peak = 0.0;
    double mean = 0.0;
    for (double value : std::as_const(combinedSignal)) {
        sumSquares += value * value;
        peak = std::max(peak, std::abs(value));
        mean += value;
    }
    mean /= combinedSignal.size();

    double variance = 0.0;
    for (double value : std::as_const(combinedSignal)) {
        const double delta = value - mean;
        variance += delta * delta;
    }
    variance /= combinedSignal.size();

    constexpr double ClassifierSampleRateHz = 100.0;
    const SignalProcessingService::FFTResult fftResult =
        m_signalProcessingService.computeFFT(combinedSignal, ClassifierSampleRateHz);
    if (fftResult.magnitudes.size() < 2)
        return features;

    features.rms = static_cast<float>(std::sqrt(sumSquares / combinedSignal.size()));
    features.peak = static_cast<float>(peak);
    features.std = static_cast<float>(std::sqrt(variance));
    features.dominantFreqHz = static_cast<float>(fftResult.dominantFrequency);
    features.valid = true;
    return features;
}

void AppController::appendLog(const QString &text)
{
    m_logs.append(QStringLiteral("%1  %2").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), text));
    while (m_logs.size() > MaxLogLines)
        m_logs.removeFirst();
    emit logTextChanged();
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

AppController::ParsedHistory AppController::parseHistorySamples(const QVector<SensorSample> &samples) const
{
    ParsedHistory parsedHistory;
    if (samples.isEmpty()) {
        parsedHistory.statusText = QStringLiteral("No stored history is available yet.");
        return parsedHistory;
    }

    QVariantList anomalyPoints;
    QVariantList rmsPoints;
    QVariantList tempPoints;
    QVariantList accelXPoints;
    QVariantList accelYPoints;
    QVariantList accelZPoints;
    QVector<double> rmsValues;
    QVector<double> tempValues;
    QVector<double> accelXValues;
    QVector<double> accelYValues;
    QVector<double> accelZValues;
    QVector<qint64> accelTimestampsMs;
    qint64 firstMs = -1;
    qint64 lastMs = -1;

    for (const SensorSample &sample : samples) {
        if (!sample.timestampUtc.isValid())
            continue;

        const qint64 pointMs = sample.timestampUtc.toUTC().toMSecsSinceEpoch();

        if (firstMs < 0)
            firstMs = pointMs;
        lastMs = pointMs;

        anomalyPoints.append(buildPoint(pointMs, sample.anomalyScore));
        rmsPoints.append(buildPoint(pointMs, sample.rms()));
        tempPoints.append(buildPoint(pointMs, sample.temp));
        accelXPoints.append(buildPoint(pointMs, sample.x));
        accelYPoints.append(buildPoint(pointMs, sample.y));
        accelZPoints.append(buildPoint(pointMs, sample.z));
        rmsValues.append(sample.rms());
        tempValues.append(sample.temp);
        accelXValues.append(sample.x);
        accelYValues.append(sample.y);
        accelZValues.append(sample.z);
        accelTimestampsMs.append(pointMs);
    }

    if (anomalyPoints.isEmpty()) {
        parsedHistory.statusText = QStringLiteral("No samples stored in the last 24 hours yet.");
        return parsedHistory;
    }

    const QVariantMap rmsRange = computePaddedRange(rmsValues, true);
    const QVariantMap tempRange = computePaddedRange(tempValues, true);
    const QVariantMap accelXRange = computePaddedRange(accelXValues, false);
    const QVariantMap accelYRange = computePaddedRange(accelYValues, false);
    const QVariantMap accelZRange = computePaddedRange(accelZValues, false);
    const qint64 safeEndMs = lastMs > firstMs ? lastMs : firstMs + 1000;

    parsedHistory.data.insert(QStringLiteral("anomalyPoints"), anomalyPoints);
    parsedHistory.data.insert(QStringLiteral("rmsPoints"), rmsPoints);
    parsedHistory.data.insert(QStringLiteral("tempPoints"), tempPoints);
    parsedHistory.data.insert(QStringLiteral("accelXPoints"), accelXPoints);
    parsedHistory.data.insert(QStringLiteral("accelYPoints"), accelYPoints);
    parsedHistory.data.insert(QStringLiteral("accelZPoints"), accelZPoints);
    parsedHistory.data.insert(QStringLiteral("anomalyMinY"), 0.0);
    parsedHistory.data.insert(QStringLiteral("anomalyMaxY"), 100.0);
    parsedHistory.data.insert(QStringLiteral("rmsMinY"), rmsRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("rmsMaxY"), rmsRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("tempMinY"), tempRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("tempMaxY"), tempRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("accelXMinY"), accelXRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("accelXMaxY"), accelXRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("accelYMinY"), accelYRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("accelYMaxY"), accelYRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("accelZMinY"), accelZRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("accelZMaxY"), accelZRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("fullStartMs"), firstMs);
    parsedHistory.data.insert(QStringLiteral("fullEndMs"), safeEndMs);
    parsedHistory.data.insert(QStringLiteral("minimumWindowMs"),
                              std::max<qint64>(1000, (safeEndMs - firstMs) / std::min<qint64>(20, anomalyPoints.size())));
    parsedHistory.data.insert(QStringLiteral("sampleCount"), anomalyPoints.size());

    if (accelXValues.size() >= HistoryFftWindowSize) {
        const int startIndex = accelXValues.size() - HistoryFftWindowSize;
        const QVector<double> fftXWindow(accelXValues.begin() + startIndex, accelXValues.end());
        const QVector<double> fftYWindow(accelYValues.begin() + startIndex, accelYValues.end());
        const QVector<double> fftZWindow(accelZValues.begin() + startIndex, accelZValues.end());
        const QVector<qint64> fftTimeWindow(accelTimestampsMs.begin() + startIndex, accelTimestampsMs.end());
        const double sampleRateHz = estimateSampleRateHz(fftTimeWindow);

        if (sampleRateHz > 0.0) {
            const SignalProcessingService::FFTResult xResult = m_signalProcessingService.computeFFT(fftXWindow, sampleRateHz);
            const SignalProcessingService::FFTResult yResult = m_signalProcessingService.computeFFT(fftYWindow, sampleRateHz);
            const SignalProcessingService::FFTResult zResult = m_signalProcessingService.computeFFT(fftZWindow, sampleRateHz);

            parsedHistory.data.insert(QStringLiteral("fftFrequencies"), toVariantList(xResult.frequencies));
            parsedHistory.data.insert(QStringLiteral("fftXMagnitudes"), toVariantList(xResult.magnitudes));
            parsedHistory.data.insert(QStringLiteral("fftYMagnitudes"), toVariantList(yResult.magnitudes));
            parsedHistory.data.insert(QStringLiteral("fftZMagnitudes"), toVariantList(zResult.magnitudes));
            parsedHistory.data.insert(QStringLiteral("fftXDominantFrequency"), xResult.dominantFrequency);
            parsedHistory.data.insert(QStringLiteral("fftYDominantFrequency"), yResult.dominantFrequency);
            parsedHistory.data.insert(QStringLiteral("fftZDominantFrequency"), zResult.dominantFrequency);
            parsedHistory.data.insert(QStringLiteral("fftXEnergy"), xResult.energy);
            parsedHistory.data.insert(QStringLiteral("fftYEnergy"), yResult.energy);
            parsedHistory.data.insert(QStringLiteral("fftZEnergy"), zResult.energy);
            parsedHistory.data.insert(QStringLiteral("fftMaxFrequency"), sampleRateHz / 2.0);
        }
    }

    parsedHistory.statusText = QStringLiteral("%1 samples loaded from the local 24-hour history database.")
                                   .arg(anomalyPoints.size());
    return parsedHistory;
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

void AppController::appendValue(QVector<double> &values, double value, int maxHistory)
{
    values.append(value);
    if (values.size() > maxHistory)
        values.removeFirst();
}

QVariantMap AppController::buildPoint(qint64 x, double y)
{
    return {
        { QStringLiteral("x"), x },
        { QStringLiteral("y"), y }
    };
}

AppController::FaultDecision AppController::classifyFault(float stdValue,
                                                          float peak,
                                                          float dominantFreqHz)
{
    FaultDecision decision;

    if (!std::isfinite(stdValue) || !std::isfinite(peak) || !std::isfinite(dominantFreqHz)) {
        decision.type = QStringLiteral("CALIBRATING");
        decision.confidence = QStringLiteral("LOW");
        decision.reason = QStringLiteral("Waiting for reliable vibration features");
        return decision;
    }

    if (stdValue > 48.784777f) {
        decision.type = QStringLiteral("BEARING_FAULT");
        decision.confidence = stdValue > 60.0f ? QStringLiteral("HIGH") : QStringLiteral("MEDIUM");
        decision.reason = QStringLiteral("High vibration variability indicates bearing damage");
        return decision;
    }

    if (dominantFreqHz <= 20.3125f) {
        decision.type = QStringLiteral("NORMAL");
        decision.confidence = QStringLiteral("MEDIUM");
        decision.reason = QStringLiteral("Dominant vibration frequency remains in the normal range");
        return decision;
    }

    if (stdValue <= 42.846477f && peak <= 692.026589f) {
        decision.type = QStringLiteral("NORMAL");
        decision.confidence = QStringLiteral("LOW");
        decision.reason = QStringLiteral("Peak and spread stay close to the learned normal pattern");
        return decision;
    }

    decision.type = QStringLiteral("IMBALANCE");
    decision.confidence = peak > 800.0f ? QStringLiteral("HIGH") : QStringLiteral("MEDIUM");
    decision.reason = QStringLiteral("Elevated peak vibration with off-normal dominant frequency");
    return decision;
}

QString AppController::toneForFaultType(const QString &faultType)
{
    if (faultType == QStringLiteral("CALIBRATING"))
        return QStringLiteral("warning");
    if (faultType == QStringLiteral("NORMAL"))
        return QStringLiteral("ok");
    if (faultType == QStringLiteral("BEARING_FAULT"))
        return QStringLiteral("fault");
    if (faultType == QStringLiteral("IMBALANCE"))
        return QStringLiteral("warning");
    if (faultType == QStringLiteral("COMMUTATOR_FAULT"))
        return QStringLiteral("warning");
    return QStringLiteral("ok");
}
