#include "serialmanager.h"

namespace {
// The UI shows a friendly label, but we still keep the raw port name separately.
QString labelFor(const QSerialPortInfo &info)
{
    return info.portName() + QStringLiteral(" (") + info.description() + QStringLiteral(")");
}
}

SerialManager::SerialManager(QObject *parent) : QObject(parent)
{
    // Every time the serial port has new bytes ready, continue parsing the incoming stream.
    connect(&m_port, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
}

QStringList SerialManager::portDisplayNames() const
{
    QStringList names;
    for (const auto &port : m_ports) names << port.label;
    return names;
}

QString SerialManager::portNameAt(int index) const
{
    return index >= 0 && index < m_ports.size() ? m_ports.at(index).name : QString();
}

void SerialManager::refreshPorts()
{
    // Rebuild the port list from scratch so removed devices disappear from the UI immediately.
    m_ports.clear();
    for (const auto &info : QSerialPortInfo::availablePorts())
        m_ports.append({info.portName(), labelFor(info)});
    emit portsChanged();
}

bool SerialManager::connectToPort(const QString &portName)
{
    const QString portNameTrimmed = portName.trimmed();
    if (portNameTrimmed.isEmpty()) return emit errorOccurred(QStringLiteral("Select a serial device first.")), false;
    if (connected()) disconnectPort();
    // These serial settings must match the firmware on the device.
    m_port.setPortName(portNameTrimmed);
    m_port.setBaudRate(QSerialPort::Baud115200);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    if (!m_port.open(QIODevice::ReadOnly)) return emit errorOccurred(m_port.errorString()), false;
    m_buffer.clear();
    emit connectedChanged();
    return true;
}

void SerialManager::disconnectPort()
{
    const bool wasOpen = connected();
    m_buffer.clear();
    if (wasOpen) m_port.close();
    if (wasOpen) emit connectedChanged();
}

void SerialManager::onReadyRead()
{
    // Append new bytes to the buffer and split on newline, because one read may contain partial or multiple messages.
    const QByteArray data = m_port.readAll();
    m_buffer.append(data);

    while (m_buffer.contains('\n')) {
        const int index = m_buffer.indexOf('\n');
        const QByteArray line = m_buffer.left(index);
        m_buffer.remove(0, index + 1);
        processLine(line);
    }
}

void SerialManager::processLine(const QByteArray &line)
{
    const QString str = QString::fromUtf8(line).trimmed();
    if (str.isEmpty())
        return;

    // Expected packet format:
    // field 0 = UID hex string, field 1 = score, field 2..4 = x/y/z,
    // field 5 = machine temperature, field 6 = ambient temperature.
    const QStringList fields = str.split(',');
    if (fields.size() != 7)
        return;

    const QString deviceId = fields[0].trimmed();
    if (!deviceId.isEmpty())
        emit deviceIdReceived(deviceId);

    bool scoreOk = false;
    bool xOk = false;
    bool yOk = false;
    bool zOk = false;
    bool tempOk = false;
    bool ambientTempOk = false;

    const double anomalyScore = fields[1].trimmed().toDouble(&scoreOk);
    const double x = fields[2].trimmed().toDouble(&xOk);
    const double y = fields[3].trimmed().toDouble(&yOk);
    const double z = fields[4].trimmed().toDouble(&zOk);
    const double temp = fields[5].trimmed().toDouble(&tempOk);
    const double ambientTemp = fields[6].trimmed().toDouble(&ambientTempOk);

    // Ignore malformed lines quietly so one bad packet does not break the live stream.
    if (!scoreOk || !xOk || !yOk || !zOk || !tempOk || !ambientTempOk)
        return;

    const QString stateText = anomalyScore >= 80.0 ? QStringLiteral("OK") : QStringLiteral("ANOMALY");
    emit packetReceived(anomalyScore, x, y, z, temp, ambientTemp, stateText);
}
