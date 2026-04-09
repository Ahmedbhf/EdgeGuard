#include "app_controller.h"

#include <QDateTime>
#include <QTextStream>
#include <QTime>
#include <QTimeZone>

#include <algorithm>
#include <cmath>

namespace {
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
    updateHistoryData(parseHistoryCsv(m_storageService.loadLast24h()));
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

AppController::ParsedHistory AppController::parseHistoryCsv(const QString &csvText) const
{
    ParsedHistory parsedHistory;
    if (csvText.trimmed().isEmpty()) {
        parsedHistory.statusText = QStringLiteral("No stored history is available yet.");
        return parsedHistory;
    }

    QString mutableCsvText = csvText;
    QTextStream stream(&mutableCsvText, QIODevice::ReadOnly);
    const QString headerLine = stream.readLine().trimmed();
    const QStringList headers = headerLine.split(',');
    const int timestampIndex = headers.indexOf(QStringLiteral("timestamp"));
    const int anomalyIndex = headers.indexOf(QStringLiteral("anomaly"));
    const int xIndex = headers.indexOf(QStringLiteral("x"));
    const int yIndex = headers.indexOf(QStringLiteral("y"));
    const int zIndex = headers.indexOf(QStringLiteral("z"));
    const int tempIndex = headers.indexOf(QStringLiteral("temp"));

    if (timestampIndex < 0 || anomalyIndex < 0 || xIndex < 0 || yIndex < 0 || zIndex < 0 || tempIndex < 0) {
        parsedHistory.statusText = QStringLiteral("Stored history format is invalid.");
        return parsedHistory;
    }

    QVariantList anomalyPoints;
    QVariantList rmsPoints;
    QVariantList tempPoints;
    QVector<double> rmsValues;
    QVector<double> tempValues;
    qint64 firstMs = -1;
    qint64 lastMs = -1;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        const QStringList fields = line.split(',');
        if (fields.size() <= std::max({timestampIndex, anomalyIndex, xIndex, yIndex, zIndex, tempIndex}))
            continue;

        bool anomalyOk = false;
        bool xOk = false;
        bool yOk = false;
        bool zOk = false;
        bool tempOk = false;

        QDateTime timestampUtc = QDateTime::fromString(fields.at(timestampIndex).trimmed(), Qt::ISODateWithMs);
        if (!timestampUtc.isValid())
            timestampUtc = QDateTime::fromString(fields.at(timestampIndex).trimmed(), Qt::ISODate);

        const double anomaly = fields.at(anomalyIndex).trimmed().toDouble(&anomalyOk);
        const double x = fields.at(xIndex).trimmed().toDouble(&xOk);
        const double y = fields.at(yIndex).trimmed().toDouble(&yOk);
        const double z = fields.at(zIndex).trimmed().toDouble(&zOk);
        const double temp = fields.at(tempIndex).trimmed().toDouble(&tempOk);

        if (!timestampUtc.isValid() || !anomalyOk || !xOk || !yOk || !zOk || !tempOk)
            continue;

        const qint64 pointMs = timestampUtc.toUTC().toMSecsSinceEpoch();
        SensorSample sample;
        sample.anomalyScore = anomaly;
        sample.x = x;
        sample.y = y;
        sample.z = z;
        sample.temp = temp;

        if (firstMs < 0)
            firstMs = pointMs;
        lastMs = pointMs;

        anomalyPoints.append(buildPoint(pointMs, anomaly));
        rmsPoints.append(buildPoint(pointMs, sample.rms()));
        tempPoints.append(buildPoint(pointMs, temp));
        rmsValues.append(sample.rms());
        tempValues.append(temp);
    }

    if (anomalyPoints.isEmpty()) {
        parsedHistory.statusText = QStringLiteral("No samples stored in the last 24 hours yet.");
        return parsedHistory;
    }

    const QVariantMap rmsRange = computePaddedRange(rmsValues, true);
    const QVariantMap tempRange = computePaddedRange(tempValues, true);
    const qint64 safeEndMs = lastMs > firstMs ? lastMs : firstMs + 1000;

    parsedHistory.data.insert(QStringLiteral("anomalyPoints"), anomalyPoints);
    parsedHistory.data.insert(QStringLiteral("rmsPoints"), rmsPoints);
    parsedHistory.data.insert(QStringLiteral("tempPoints"), tempPoints);
    parsedHistory.data.insert(QStringLiteral("anomalyMinY"), 0.0);
    parsedHistory.data.insert(QStringLiteral("anomalyMaxY"), 100.0);
    parsedHistory.data.insert(QStringLiteral("rmsMinY"), rmsRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("rmsMaxY"), rmsRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("tempMinY"), tempRange.value(QStringLiteral("min")));
    parsedHistory.data.insert(QStringLiteral("tempMaxY"), tempRange.value(QStringLiteral("max")));
    parsedHistory.data.insert(QStringLiteral("fullStartMs"), firstMs);
    parsedHistory.data.insert(QStringLiteral("fullEndMs"), safeEndMs);
    parsedHistory.data.insert(QStringLiteral("minimumWindowMs"),
                              std::max<qint64>(1000, (safeEndMs - firstMs) / std::min<qint64>(20, anomalyPoints.size())));
    parsedHistory.data.insert(QStringLiteral("sampleCount"), anomalyPoints.size());
    parsedHistory.statusText =
        QStringLiteral("%1 samples loaded from the rolling 24-hour store. Hover to inspect points, drag horizontally to scroll, and use the mouse wheel to zoom.")
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
