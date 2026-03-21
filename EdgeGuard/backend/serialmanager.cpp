#include "serialmanager.h"

namespace {
QString labelFor(const QSerialPortInfo &info)
{
    return info.portName() + QStringLiteral(" (") + info.description() + QStringLiteral(")");
}
}

SerialManager::SerialManager(QObject *parent) : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, &SerialManager::onError);
    m_timeout.setInterval(2000);
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, &SerialManager::onTimeout);
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
    m_port.setPortName(portNameTrimmed);
    m_port.setBaudRate(QSerialPort::Baud115200);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    if (!m_port.open(QIODevice::ReadOnly)) return emit errorOccurred(m_port.errorString()), false;
    m_buffer.clear();
    m_timeout.start();
    emit connectedChanged();
    return true;
}

void SerialManager::disconnectPort()
{
    const bool wasOpen = connected();
    m_timeout.stop();
    m_buffer.clear();
    if (wasOpen) m_port.close();
    if (wasOpen) emit connectedChanged();
}

void SerialManager::onReadyRead()
{
    m_buffer += m_port.readAll();
    for (int end = m_buffer.indexOf('\n'); end >= 0; end = m_buffer.indexOf('\n')) {
        const QByteArray line = m_buffer.left(end).trimmed();
        m_buffer.remove(0, end + 1);
        if (!line.isEmpty()) processLine(line);
    }
    if (connected()) m_timeout.start();
}

void SerialManager::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    const QString message = m_port.errorString();
    if (error == QSerialPort::ResourceError) { disconnectPort(); refreshPorts(); }
    emit errorOccurred(message);
}

void SerialManager::onTimeout() { emit errorOccurred(QStringLiteral("No data received (timeout).")); }

void SerialManager::processLine(const QByteArray &line)
{
    const QString text = QString::fromUtf8(line);
    const QStringList fields = text.split(',');
    if (fields.size() != 7)
        return;

    bool okX = false;
    bool okY = false;
    bool okZ = false;
    bool okTemp = false;

    const QString stateText = fields[2].trimmed();
    const double x = fields[3].trimmed().toDouble(&okX);
    const double y = fields[4].trimmed().toDouble(&okY);
    const double z = fields[5].trimmed().toDouble(&okZ);
    const double temp = fields[6].trimmed().toDouble(&okTemp);

    if (!okX || !okY || !okZ || !okTemp || stateText.isEmpty())
        return;

    emit packetReceived(x, y, z, temp, stateText);
}
