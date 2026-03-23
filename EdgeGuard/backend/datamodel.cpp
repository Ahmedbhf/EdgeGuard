#include "datamodel.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QTime>
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
DataModel::DataModel(QObject *parent) : QObject(parent), m_serial(new SerialManager(this))
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
    appendLog(QStringLiteral("Ready."));
}
DataModel::~DataModel()
{
    stopCsv();
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

void DataModel::openCsvFile()
{
    // Open the saved file directly when possible, otherwise open its folder as a fallback.
    if (m_csvPath.isEmpty() || !QFileInfo::exists(m_csvPath))
        return appendLog(QStringLiteral("No CSV file available yet."));
    const QUrl fileUrl = QUrl::fromLocalFile(m_csvPath);
    if (QDesktopServices::openUrl(fileUrl)) return appendLog(QStringLiteral("Opened CSV file."));
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_csvPath).absolutePath())))
        appendLog(QStringLiteral("Could not open the CSV file."));
}

void DataModel::startLogging()
{
    // Logging is only useful when live data is already coming from the serial device.
    if (!connected())
        return appendLog(QStringLiteral("Connect to a UART port before starting logging."));
    if (m_loggingEnabled)
        return;
    startCsv();
    if (m_loggingEnabled)
        appendLog(QStringLiteral("Logging started"));
}

void DataModel::stopLogging()
{
    if (!m_loggingEnabled)
        return;
    stopCsv();
    appendLog(QStringLiteral("Logging stopped"));
}

QString DataModel::readTextFileLimited(const QUrl &fileUrl, int maxLines) const
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    QTextStream stream(&file);
    QStringList lines;
    int count = 0;
    // This is used by QML previews where loading the full file is not necessary.
    while (!stream.atEnd() && count < maxLines) {
        lines.append(stream.readLine());
        ++count;
    }

    return lines.join('\n');
}

void DataModel::onPacketReceived(double anomalyScore, double x, double y, double z, double temp, const QString &stateText)
{
    processSample(anomalyScore, x, y, z, temp, stateText);
}

void DataModel::processSample(double anomalyScore, double x, double y, double z, double temp, const QString &stateText)
{
    // Store the newest raw values first so all later calculations use the same sample.
    const QString nextState = stateText.trimmed();
    const bool stateUpdated = nextState != m_state;
    m_state = nextState;
    m_anomalyScore = anomalyScore;
    m_x = x; m_y = y; m_z = z; m_temp = temp;
    updateMetrics();
    // These accumulators let us average short bursts of samples before refreshing the charts.
    m_windowRmsSquareSum += (m_rms * m_rms);
    m_windowTempSum += m_temp;
    ++m_windowSampleCount;
    m_pendingUiRefresh = true;
    writeCsv();
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
        if (m_loggingEnabled)
            stopCsv();
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

        trim(m_vibration, aggregatedRms, MaxHistory);
        trim(m_temperature, aggregatedTemp, MaxHistory);
        emit vibrationValuesChanged();
        emit temperatureValuesChanged();
    }

    m_windowSampleCount = 0;
    m_windowRmsSquareSum = 0.0;
    m_windowTempSum = 0.0;

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

void DataModel::startCsv()
{
    stopCsv();
    // Save logs in the user's Documents folder so the file is easy to find after the session.
    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty()) base = QDir::currentPath();
    QDir(base).mkpath(QStringLiteral("EdgeGuard"));
    m_csvPath = QDir(base).filePath(QStringLiteral("EdgeGuard/edgeguard_%1.csv").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))));
    m_csv.setFileName(m_csvPath);
    if (!m_csv.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_csvPath.clear();
        emit csvFilePathChanged();
        return appendLog(QStringLiteral("Could not create CSV file."));
    }
    m_loggingEnabled = true;
    emit loggingEnabledChanged();
    emit csvFilePathChanged();
    m_csvWritesSinceFlush = 0;
    m_csv.write("time,rms,temp,state\n");
    m_csv.flush();
    appendLog(QStringLiteral("CSV: %1").arg(m_csvPath));
}

void DataModel::stopCsv()
{
    if (m_csv.isOpen()) {
        m_csv.flush();
        m_csv.close();
    }
    if (m_loggingEnabled) {
        m_loggingEnabled = false;
        emit loggingEnabledChanged();
    }
    m_csvWritesSinceFlush = 0;
}

void DataModel::writeCsv()
{
    if (!m_loggingEnabled) return;
    if (!m_csv.isOpen()) return;
    // We write already-calculated values instead of raw axes because the dashboard shows the same summary values.
    const QString line = QStringLiteral("%1,%2,%3,%4\n")
                             .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")))
                             .arg(QString::number(m_rms, 'f', 4))
                             .arg(QString::number(m_temp, 'f', 1))
                             .arg(m_state);
    m_csv.write(line.toUtf8());
    ++m_csvWritesSinceFlush;
    if (m_csvWritesSinceFlush >= CsvFlushInterval) {
        m_csv.flush();
        m_csvWritesSinceFlush = 0;
    }
}
