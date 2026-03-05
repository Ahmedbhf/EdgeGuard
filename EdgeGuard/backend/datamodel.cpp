#include "datamodel.h"
#include <cmath>
#include <QDebug>

DataModel::DataModel(QObject *parent)
    : QObject(parent),
    m_serial(new QSerialPort(this)),
    m_timeoutTimer(new QTimer(this)),
    m_uiTimer(new QTimer(this)),
    m_motorId(0),
    m_motorStatus(0),
    m_x(0), m_y(0), m_z(0),
    m_rms(0), m_peak2peak(0), m_variance(0), m_crestFactor(0),
    m_temp(0), m_tempSlope(0), m_lastTemp(0),
    m_zScore(0), m_meanRms(1.10), m_stdRms(0.15)
{
    connect(m_serial, &QSerialPort::readyRead, this, &DataModel::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &DataModel::onSerialError);

    // Timeout: if no data received for 2 seconds, consider link lost
    m_timeoutTimer->setInterval(2000);
    m_timeoutTimer->setSingleShot(false);
    connect(m_timeoutTimer, &QTimer::timeout, this, &DataModel::onTimeoutTick);

    // Throttle UI metric updates to 500 ms so text is readable
    m_uiTimer->setInterval(500);
    connect(m_uiTimer, &QTimer::timeout, this, [this]() {
        emit dataChanged();
    });
}

// ── Serial port management ──────────────────────────────────────────

void DataModel::connectToPort(const QString &portName)
{
    if (m_serial->isOpen())
        m_serial->close();

    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadOnly)) {
        m_readBuffer.clear();
        m_timeoutTimer->start();
        m_uiTimer->start();
        qDebug() << "Serial connected:" << portName;
        emit connectedChanged();
    } else {
        emit serialError(m_serial->errorString());
    }
}

void DataModel::disconnectPort()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        m_timeoutTimer->stop();
        m_uiTimer->stop();
        m_readBuffer.clear();
        qDebug() << "Serial disconnected";
        emit connectedChanged();
    }
}

void DataModel::refreshPorts()
{
    emit availablePortsChanged();
}

bool DataModel::connected() const
{
    return m_serial->isOpen();
}

QStringList DataModel::availablePorts() const
{
    QStringList list;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
        list << info.portName();
    return list;
}

QString DataModel::currentPort() const
{
    return m_serial->portName();
}

int DataModel::motorId() const     { return m_motorId; }
int DataModel::motorStatus() const { return m_motorStatus; }

// ── Data reception ──────────────────────────────────────────────────

void DataModel::onReadyRead()
{
    m_readBuffer.append(m_serial->readAll());

    // Process complete lines delimited by \r\n
    while (true) {
        int idx = m_readBuffer.indexOf("\r\n");
        if (idx == -1)
            break;
        QByteArray line = m_readBuffer.left(idx).trimmed();
        m_readBuffer.remove(0, idx + 2);

        if (!line.isEmpty())
            processLine(line);
    }

    // Guard against unbounded buffer growth from garbage data
    if (m_readBuffer.size() > 4096)
        m_readBuffer.clear();

    // Reset timeout on any data arrival
    m_timeoutTimer->start();
}

void DataModel::processLine(const QByteArray &line)
{
    QList<QByteArray> fields = line.split(',');
    if (fields.size() != 6)
        return; // Malformed — ignore silently

    bool ok1, ok2, ok3, ok4, ok5, ok6;
    int    idM     = fields[0].toInt(&ok1);
    int    status  = fields[1].toInt(&ok2);
    double accelX  = fields[2].toDouble(&ok3);
    double accelY  = fields[3].toDouble(&ok4);
    double accelZ  = fields[4].toDouble(&ok5);
    double tempC   = fields[5].toDouble(&ok6);

    if (!(ok1 && ok2 && ok3 && ok4 && ok5 && ok6))
        return; // Parse failure — ignore

    m_motorId     = idM;
    m_motorStatus = status;
    m_x = accelX;
    m_y = accelY;
    m_z = accelZ;
    m_temp = tempC;

    compute();

    // Append to chart history (runs at frame rate ≈ 10 Hz from STM32)
    m_vibrationValues.append(m_rms);
    if (m_vibrationValues.size() > MAX_HISTORY)
        m_vibrationValues.removeFirst();
    emit vibrationValuesChanged();

    m_temperatureValues.append(m_temp);
    if (m_temperatureValues.size() > MAX_HISTORY)
        m_temperatureValues.removeFirst();
    emit temperatureValuesChanged();
}

// ── Error / timeout handling ────────────────────────────────────────

void DataModel::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    QString msg = m_serial->errorString();
    qWarning() << "Serial error:" << msg;
    emit serialError(msg);

    if (error == QSerialPort::ResourceError) {
        // Device unplugged
        disconnectPort();
    }
}

void DataModel::onTimeoutTick()
{
    qWarning() << "Serial timeout — no data for 2 s";
    emit serialError(QStringLiteral("No data received (timeout)"));
}

// ── Getters ─────────────────────────────────────────────────────────

double DataModel::rms() const         { return m_rms; }
double DataModel::peak2peak() const   { return m_peak2peak; }
double DataModel::variance() const    { return m_variance; }
double DataModel::crestFactor() const { return m_crestFactor; }
double DataModel::temp() const        { return m_temp; }
double DataModel::tempSlope() const   { return m_tempSlope; }
double DataModel::zScore() const      { return m_zScore; }

// ── Compute derived metrics (unchanged algorithm) ───────────────────

void DataModel::compute()
{
    m_rms = std::sqrt((m_x*m_x + m_y*m_y + m_z*m_z) / 3.0);

    if (m_stdRms != 0)
        m_zScore = (m_rms - m_meanRms) / m_stdRms;
    else
        m_zScore = 0;

    m_peak2peak = std::max({m_x, m_y, m_z}) - std::min({m_x, m_y, m_z});

    m_variance = (m_x*m_x + m_y*m_y + m_z*m_z) / 3.0 - (m_rms * m_rms);

    if (m_rms != 0)
        m_crestFactor = m_peak2peak / m_rms;
    else
        m_crestFactor = 0;

    m_tempSlope = m_temp - m_lastTemp;
    m_lastTemp = m_temp;
}
