#include "datamodel.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTime>
#include <QUrl>
#include <algorithm>
#include <cmath>

namespace {
void trim(QVector<double> &values, double value, int max)
{
    values.append(value);
    if (values.size() > max)
        values.removeFirst();
}
}
DataModel::DataModel(QObject *parent) : QObject(parent), m_serial(new SerialManager(this))
{
    connect(m_serial, &SerialManager::packetReceived, this, &DataModel::onPacketReceived);
    connect(m_serial, &SerialManager::errorOccurred, this, [this](const QString &text) { appendLog(text); });
    connect(m_serial, &SerialManager::portsChanged, this, &DataModel::syncPorts);
    connect(m_serial, &SerialManager::connectedChanged, this, &DataModel::syncConnection);
    refreshPorts();
    appendLog(QStringLiteral("Ready."));
}
DataModel::~DataModel()
{
    stopCsv();
}

void DataModel::connectToPort(const QString &portName)
{
    const QString port = portName.split(' ').first().trimmed();
    if (port.isEmpty()) return appendLog(QStringLiteral("No serial device selected."));
    setSelectedPort(port);
    if (!m_serial->connectToPort(port)) return;
    appendLog(QStringLiteral("Connected to %1").arg(port));
}
void DataModel::disconnectPort() { if (connected()) m_serial->disconnectPort(); }
void DataModel::toggleConnection() { connected() ? disconnectPort() : connectToPort(m_selectedPort); }

void DataModel::setSelectedPort(const QString &portName)
{
    const QString port = portName.trimmed();
    if (m_selectedPort == port) return;
    m_selectedPort = port;
    emit selectedPortChanged();
}
void DataModel::openCsvFile()
{
    if (m_csvPath.isEmpty() || !QFileInfo::exists(m_csvPath))
        return appendLog(QStringLiteral("No CSV file available yet."));
    const QUrl fileUrl = QUrl::fromLocalFile(m_csvPath);
    if (QDesktopServices::openUrl(fileUrl)) return appendLog(QStringLiteral("Opened CSV file."));
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_csvPath).absolutePath())))
        appendLog(QStringLiteral("Could not open the CSV file."));
}

void DataModel::startLogging()
{
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

QString DataModel::readTextFile(const QUrl &fileUrl) const
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

void DataModel::onPacketReceived(double x, double y, double z, double temp, const QString &, int status)
{
    const QString nextState = status == 0 ? QStringLiteral("OK") : QStringLiteral("ANOMALY");
    const bool stateUpdated = nextState != m_state;
    m_state = nextState;
    m_x = x; m_y = y; m_z = z; m_temp = temp;
    updateMetrics();
    appendHistory();
    writeCsv();
    if (stateUpdated) { emit stateChanged(); appendLog(QStringLiteral("State: %1").arg(m_state)); }
    emit dataChanged();
}

void DataModel::syncPorts()
{
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
        if (m_loggingEnabled)
            stopCsv();
        appendLog(QStringLiteral("Disconnected from %1").arg(m_selectedPort.isEmpty() ? QStringLiteral("device") : m_selectedPort));
    }
    emit connectedChanged();
}

void DataModel::updateMetrics()
{
    const double mean = (m_x + m_y + m_z) / 3.0;
    const double ax = m_x - mean, ay = m_y - mean, az = m_z - mean;
    const double energy = (ax * ax + ay * ay + az * az) / 3.0;
    m_rms = std::sqrt(energy);
    m_peak2peak = std::max({m_x, m_y, m_z}) - std::min({m_x, m_y, m_z});
    m_variance = energy - (m_rms * m_rms);
    m_crestFactor = m_rms == 0.0 ? 0.0 : m_peak2peak / m_rms;
    m_tempSlope = m_temp - m_lastTemp;
    m_lastTemp = m_temp;
}

void DataModel::appendHistory()
{
    trim(m_vibration, m_rms, MaxHistory);
    trim(m_temperature, m_temp, MaxHistory);
    emit vibrationValuesChanged();
    emit temperatureValuesChanged();
}

void DataModel::appendLog(const QString &text)
{
    m_logs.append(QStringLiteral("%1  %2").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), text));
    while (m_logs.size() > MaxLogLines) m_logs.removeFirst();
    emit logTextChanged();
}

void DataModel::startCsv()
{
    stopCsv();
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
    m_csv.write("time,rms,peak2peak,variance,temp,state\n");
    m_csv.flush();
    appendLog(QStringLiteral("CSV: %1").arg(m_csvPath));
}

void DataModel::stopCsv()
{
    if (m_csv.isOpen())
        m_csv.close();
    if (m_loggingEnabled) {
        m_loggingEnabled = false;
        emit loggingEnabledChanged();
    }
}

void DataModel::writeCsv()
{
    if (!m_loggingEnabled) return;
    if (!m_csv.isOpen()) return;
    const QString line = QStringLiteral("%1,%2,%3,%4,%5,%6\n")
                             .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")))
                             .arg(QString::number(m_rms, 'f', 4))
                             .arg(QString::number(m_peak2peak, 'f', 4))
                             .arg(QString::number(m_variance, 'f', 4))
                             .arg(QString::number(m_temp, 'f', 1))
                             .arg(m_state);
    m_csv.write(line.toUtf8());
    m_csv.flush();
}
