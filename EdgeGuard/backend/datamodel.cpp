#include "datamodel.h"
#include "data_storage.h"

#include <QDateTime>
#include <QTime>
#include <QTimeZone>
#include <cmath>

namespace {
// Appends a new sample and keeps only the latest "max" values for chart history.
void trim(QVector<double> &values, double value, int max)
{
    values.append(value);
    if (values.size() > max)
        values.removeFirst();
}
}
DataModel::DataModel(QObject *parent)
    : QObject(parent),
      m_serial(new SerialManager(this)),
      m_storage(std::make_unique<DataStorage>())
{
    // The device may send its identity before the user chooses anything in the UI.
    connect(m_serial, &SerialManager::deviceIdReceived, this, [this](const QString &deviceId) {
        if (m_deviceId.isEmpty())
            setDeviceId(deviceId);
        if (m_machineType.isEmpty())
            setMachineType(QStringLiteral("DC Motor"));
    });
    connect(m_serial, &SerialManager::packetReceived, this, &DataModel::onPacketReceived);
    connect(m_serial, &SerialManager::errorOccurred, this, [this](const QString &text) { appendLog(text); });
    connect(m_serial, &SerialManager::portsChanged, this, &DataModel::syncPorts);
    connect(m_serial, &SerialManager::connectedChanged, this, &DataModel::syncConnection);
    // UI charts are updated on a timer so bursts of serial data do not redraw the screen too often.
    m_uiSampleTimer.setInterval(UiAggregationIntervalMs);
    connect(&m_uiSampleTimer, &QTimer::timeout, this, &DataModel::flushUiSamples);
    m_uiSampleTimer.start();
    refreshPorts();
    m_lastStoredSampleMs = QDateTime::currentMSecsSinceEpoch();
    appendLog(QStringLiteral("Ready."));
}
DataModel::~DataModel()
{
}

void DataModel::connectToPort(const QString &portName)
{
    // QML may pass a display label like "COM11 (USB Serial Device)", so keep only the real port name.
    const QString port = portName.split(' ').first().trimmed();
    if (port.isEmpty()) return appendLog(QStringLiteral("No serial device selected."));
    setSelectedPort(port);
    if (!m_serial->connectToPort(port)) return;
    appendLog(QStringLiteral("Connected to %1").arg(port));
}
void DataModel::disconnectPort() { if (connected()) m_serial->disconnectPort(); }

void DataModel::setSelectedPort(const QString &portName)
{
    const QString port = portName.trimmed();
    if (m_selectedPort == port) return;
    m_selectedPort = port;
    emit selectedPortChanged();
}

void DataModel::setMachineType(const QString &machineType)
{
    const QString trimmedMachineType = machineType.trimmed();
    if (m_machineType == trimmedMachineType)
        return;
    m_machineType = trimmedMachineType;
    emit machineTypeChanged();
}

void DataModel::setDeviceId(const QString &deviceId)
{
    const QString trimmedDeviceId = deviceId.trimmed();
    if (m_deviceId == trimmedDeviceId)
        return;
    m_deviceId = trimmedDeviceId;
    emit deviceIdChanged();
}

QString DataModel::loadLast24hCsv() const
{
    return m_storage ? m_storage->loadLast24h() : QString();
}

bool DataModel::exportHistoryCsv(const QUrl &fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    const bool ok = m_storage && m_storage->exportCsv(path);
    appendLog(ok
                  ? QStringLiteral("Exported 24h history to %1").arg(path)
                  : QStringLiteral("Could not export the 24h history CSV."));
    return ok;
}

void DataModel::onPacketReceived(double anomalyScore, double x, double y, double z, double temp, double ambientTemp, const QString &stateText)
{
    processSample(anomalyScore, x, y, z, temp, ambientTemp, stateText);
}

void DataModel::processSample(double anomalyScore, double x, double y, double z, double temp, double ambientTemp, const QString &stateText)
{
    // Store the newest raw values first so all later calculations use the same sample.
    const QString nextState = stateText.trimmed();
    const bool stateUpdated = nextState != m_state;
    m_state = nextState;
    m_anomalyScore = anomalyScore;
    m_x = x; m_y = y; m_z = z; m_temp = temp; m_ambientTemp = ambientTemp;
    updateMetrics();
    // These accumulators let us average short bursts of samples before refreshing the charts.
    m_windowRmsSquareSum += (m_rms * m_rms);
    m_windowTempSum += m_temp;
    m_windowAnomalySum += m_anomalyScore;
    m_windowXSum += m_x;
    m_windowYSum += m_y;
    m_windowZSum += m_z;
    ++m_windowSampleCount;
    m_pendingUiRefresh = true;
    if (stateUpdated) { emit stateChanged(); appendLog(QStringLiteral("State: %1").arg(m_state)); }
}

void DataModel::syncPorts()
{
    // Keep the previous selection when possible; otherwise fall back to the first detected port.
    emit availablePortsChanged();
    QString next = m_selectedPort;
    bool keep = false;
    for (int i = 0; i < availablePorts().size(); ++i) keep |= portNameAt(i) == m_selectedPort;
    if (!keep) next = availablePorts().isEmpty() ? QString() : portNameAt(0);
    if (next != m_selectedPort) { m_selectedPort = next; emit selectedPortChanged(); }
}

void DataModel::syncConnection()
{
    if (!connected()) {
        // Flush any pending chart values before the session fully ends.
        flushUiSamples();
        appendLog(QStringLiteral("Disconnected from %1").arg(m_selectedPort.isEmpty() ? QStringLiteral("device") : m_selectedPort));
    }
    emit connectedChanged();
}

void DataModel::updateMetrics()
{
    // RMS is calculated from the three vibration axes to give one overall vibration level.
    const double energy = ((m_x * m_x) + (m_y * m_y) + (m_z * m_z)) / 3.0;
    m_rms = std::sqrt(energy);
}

void DataModel::flushUiSamples()
{
    if (!m_pendingUiRefresh && m_windowSampleCount == 0)
        return;

    if (m_windowSampleCount > 0) {
        // Average the short window so the chart shows smoother values and fewer redraws.
        const double aggregatedRms = std::sqrt(m_windowRmsSquareSum / m_windowSampleCount);
        const double aggregatedTemp = m_windowTempSum / m_windowSampleCount;
        const double aggregatedAnomalyScore = m_windowAnomalySum / m_windowSampleCount;
        const double aggregatedX = m_windowXSum / m_windowSampleCount;
        const double aggregatedY = m_windowYSum / m_windowSampleCount;
        const double aggregatedZ = m_windowZSum / m_windowSampleCount;

        trim(m_vibration, aggregatedRms, MaxHistory);
        trim(m_temperature, aggregatedTemp, MaxHistory);
        trim(m_xHistory, aggregatedX, MaxHistory);
        trim(m_yHistory, aggregatedY, MaxHistory);
        trim(m_zHistory, aggregatedZ, MaxHistory);
        emit vibrationValuesChanged();
        emit temperatureValuesChanged();
        emit xAxisValuesChanged();
        emit yAxisValuesChanged();
        emit zAxisValuesChanged();

        storeHistoryPoint(aggregatedAnomalyScore, aggregatedX, aggregatedY, aggregatedZ, aggregatedTemp);
    }

    m_windowSampleCount = 0;
    m_windowRmsSquareSum = 0.0;
    m_windowTempSum = 0.0;
    m_windowAnomalySum = 0.0;
    m_windowXSum = 0.0;
    m_windowYSum = 0.0;
    m_windowZSum = 0.0;

    if (m_pendingUiRefresh)
        emit dataChanged();

    m_pendingUiRefresh = false;
}

void DataModel::appendLog(const QString &text)
{
    // Prefix each log line with a clock time so the operator can follow the sequence of events.
    m_logs.append(QStringLiteral("%1  %2").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), text));
    while (m_logs.size() > MaxLogLines) m_logs.removeFirst();
    emit logTextChanged();
}

void DataModel::storeHistoryPoint(double anomalyScore, double x, double y, double z, double temp)
{
    if (!m_storage)
        return;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs - m_lastStoredSampleMs < StorageIntervalMs)
        return;

    DataStorage::DataPoint point;
    point.timestampUtc = QDateTime::fromMSecsSinceEpoch(nowMs, QTimeZone::UTC);
    point.anomaly = anomalyScore;
    point.x = x;
    point.y = y;
    point.z = z;
    point.temp = temp;
    m_storage->appendData(point);
    m_lastStoredSampleMs = nowMs;

    ++m_storageSamplesSinceCleanup;
    if (m_storageSamplesSinceCleanup >= StorageCleanupIntervalSamples) {
        m_storage->cleanOldData();
        m_storageSamplesSinceCleanup = 0;
    }
}
