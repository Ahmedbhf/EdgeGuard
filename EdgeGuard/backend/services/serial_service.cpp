#include "serial_service.h"

namespace {
QString labelFor(const QSerialPortInfo &info)
{
    return info.portName() + QStringLiteral(" (") + info.description() + QStringLiteral(")");
}
}

SerialService::SerialService(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<SensorSample>("SensorSample");
    connect(&m_port, &QSerialPort::readyRead, this, &SerialService::onReadyRead);
}

QStringList SerialService::portDisplayNames() const
{
    QStringList names;
    for (const auto &port : m_ports)
        names << port.label;
    return names;
}

QString SerialService::portNameAt(int index) const
{
    return index >= 0 && index < m_ports.size() ? m_ports.at(index).name : QString();
}

void SerialService::refreshPorts()
{
    m_ports.clear();
    for (const auto &info : QSerialPortInfo::availablePorts())
        m_ports.append({info.portName(), labelFor(info)});
    emit portsChanged();
}

bool SerialService::connectToPort(const QString &portName)
{
    const QString trimmedPortName = portName.trimmed();
    if (trimmedPortName.isEmpty())
        return emit errorOccurred(QStringLiteral("Select a serial device first.")), false;

    if (connected())
        disconnectPort();

    m_port.setPortName(trimmedPortName);
    m_port.setBaudRate(QSerialPort::Baud115200);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadOnly))
        return emit errorOccurred(m_port.errorString()), false;

    m_buffer.clear();
    emit connectedChanged();
    return true;
}

void SerialService::disconnectPort()
{
    const bool wasOpen = connected();
    m_buffer.clear();
    if (wasOpen)
        m_port.close();
    if (wasOpen)
        emit connectedChanged();
}

void SerialService::onReadyRead()
{
    m_buffer.append(m_port.readAll());

    while (m_buffer.contains('\n')) {
        const int newlineIndex = m_buffer.indexOf('\n');
        const QByteArray line = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);
        processLine(line);
    }
}

void SerialService::processLine(const QByteArray &line)
{
    const QString textLine = QString::fromUtf8(line).trimmed();
    if (textLine.isEmpty())
        return;

    const QStringList fields = textLine.split(',');
    if (fields.size() != 7)
        return;

    SensorSample sample;
    sample.deviceId = fields[0].trimmed();
    if (!sample.deviceId.isEmpty())
        emit deviceIdReceived(sample.deviceId);

    bool scoreOk = false;
    bool xOk = false;
    bool yOk = false;
    bool zOk = false;
    bool tempOk = false;
    bool ambientOk = false;

    sample.anomalyScore = fields[1].trimmed().toDouble(&scoreOk);
    sample.x = fields[2].trimmed().toDouble(&xOk);
    sample.y = fields[3].trimmed().toDouble(&yOk);
    sample.z = fields[4].trimmed().toDouble(&zOk);
    sample.temp = fields[5].trimmed().toDouble(&tempOk);
    sample.ambientTemp = fields[6].trimmed().toDouble(&ambientOk);

    if (!scoreOk || !xOk || !yOk || !zOk || !tempOk || !ambientOk)
        return;

    sample.timestampUtc = QDateTime::currentDateTimeUtc();
    sample.state = SensorSample::stateForScore(sample.anomalyScore);
    emit sampleReceived(sample);
}
