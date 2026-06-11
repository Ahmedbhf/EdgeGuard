#include "serial_service.h"

namespace {
// Builds the visible label used in the setup port picker.
QString labelFor(const QSerialPortInfo &info)
{
    return info.portName() + QStringLiteral(" (") + info.description() + QStringLiteral(")");
}

// Parses a floating-point field from a comma-separated UART frame.
bool readDoubleField(const QStringList &fields, int index, double &value)
{
    bool ok = false;
    value = fields[index].trimmed().toDouble(&ok);
    return ok;
}

// Parses an integer field from a comma-separated UART frame.
bool readIntField(const QStringList &fields, int index, int &value)
{
    bool ok = false;
    value = fields[index].trimmed().toInt(&ok);
    return ok;
}

// Parses the state field, accepting both raw numbers and "etat:" prefixes.
bool readStateField(const QStringList &fields, int index, int &value)
{
    QString stateText = fields[index].trimmed();
    const QString prefix = QStringLiteral("etat:");

    if (stateText.startsWith(prefix, Qt::CaseInsensitive))
        stateText = stateText.mid(prefix.size()).trimmed();

    bool ok = false;
    value = stateText.toInt(&ok);
    return ok;
}
}

// Registers SensorSample and wires serial ready-read events to the parser.
SerialService::SerialService(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<SensorSample>("SensorSample");
    connect(&m_port, &QSerialPort::readyRead, this, &SerialService::onReadyRead);
}

// Returns user-facing labels for each detected serial port.
QStringList SerialService::portDisplayNames() const
{
    QStringList names;
    for (const auto &port : m_ports)
        names << port.label;
    return names;
}

// Returns the raw port name behind a visible port list index.
QString SerialService::portNameAt(int index) const
{
    return index >= 0 && index < m_ports.size() ? m_ports.at(index).name : QString();
}

// Re-scans available serial ports and notifies QML of the new list.
void SerialService::refreshPorts()
{
    m_ports.clear();
    for (const auto &info : QSerialPortInfo::availablePorts())
        m_ports.append({info.portName(), labelFor(info)});
    emit portsChanged();
}

// Configures and opens a UART connection to the requested port.
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

    if (!m_port.open(QIODevice::ReadWrite))
        return emit errorOccurred(m_port.errorString()), false;

    m_buffer.clear();
    emit connectedChanged();
    return true;
}

// Closes the UART port and clears any partially received frame.
void SerialService::disconnectPort()
{
    const bool wasOpen = connected();
    m_buffer.clear();
    if (wasOpen)
        m_port.close();
    if (wasOpen)
        emit connectedChanged();
}

// Sends a command to the connected device and reports write failures.
bool SerialService::writeData(const QByteArray &data)
{
    if (!connected())
        return emit errorOccurred(QStringLiteral("Serial device is not connected.")), false;
    if (data.isEmpty())
        return false;

    const qint64 bytesWritten = m_port.write(data);
    if (bytesWritten != data.size()) {
        emit errorOccurred(QStringLiteral("Could not write the full UART command."));
        return false;
    }

    m_port.flush();
    return true;
}

// Buffers incoming bytes and dispatches complete newline-terminated frames.
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

// Parses one UART CSV frame into a SensorSample and emits it to the app.
void SerialService::processLine(const QByteArray &line)
{
    const QString textLine = QString::fromUtf8(line).trimmed();
    if (textLine.isEmpty())
        return;

    const QStringList fields = textLine.split(',');
    if (fields.size() < 7)
        return;

    SensorSample sample;
    sample.deviceId = fields[0].trimmed();
    if (!sample.deviceId.isEmpty())
        emit deviceIdReceived(sample.deviceId);

    const bool parsed =
        readDoubleField(fields, 1, sample.anomalyScore)
        && readDoubleField(fields, 2, sample.x)
        && readDoubleField(fields, 3, sample.y)
        && readDoubleField(fields, 4, sample.z)
        && readDoubleField(fields, 5, sample.temp)
        && readDoubleField(fields, 6, sample.ambientTemp);
    if (!parsed)
        return;

    if (fields.size() >= 8) {
        int stateCode = -1;
        if (!readStateField(fields, 7, stateCode))
            return;

        sample.state = SensorSample::stateForCode(stateCode);
        if (sample.state.isEmpty())
            return;
    } else {
        sample.state = SensorSample::stateForScore(sample.anomalyScore);
    }

    if (fields.size() >= 9) {
        int classCode = -1;
        if (!readIntField(fields, 8, classCode))
            return;

        sample.operatingCondition = SensorSample::operatingConditionForCode(classCode, sample.state);
        if (sample.operatingCondition.isEmpty())
            return;
    } else {
        sample.operatingCondition = sample.state == QStringLiteral("NORMAL")
                                        ? QStringLiteral("Motor On")
                                        : QStringLiteral("Monitoring");
    }

    bool isLearning = false;
    int nlValue = 0;
    for (int i = 7; i < fields.size(); ++i) {
        QString f = fields[i].trimmed();
        if (f.startsWith(QStringLiteral("l:"), Qt::CaseInsensitive)) {
            bool ok = false;
            int val = f.mid(2).trimmed().toInt(&ok);
            if (ok && val == 1) {
                isLearning = true;
            }
        } else if (f.startsWith(QStringLiteral("nl:"), Qt::CaseInsensitive)) {
            bool ok = false;
            int val = f.mid(3).trimmed().toInt(&ok);
            if (ok) {
                nlValue = val;
            }
        }
    }

    sample.isLearning = isLearning;
    sample.nlValue = nlValue;

    if (isLearning) {
        sample.state = QStringLiteral("LEARNING");
        sample.operatingCondition = QString::number(nlValue) + QStringLiteral("%");
    }

    sample.timestampUtc = QDateTime::currentDateTimeUtc();
    emit sampleReceived(sample);
}
