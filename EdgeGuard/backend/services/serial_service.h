#ifndef SERIAL_SERVICE_H
#define SERIAL_SERVICE_H

#include "../models/sensor_sample.h"

#include <QObject>
#include <QByteArray>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QVector>

class SerialService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(QStringList portDisplayNames READ portDisplayNames NOTIFY portsChanged)

public:
    explicit SerialService(QObject *parent = nullptr);

    // Reports whether the UART port is currently open.
    bool connected() const { return m_port.isOpen(); }
    // Returns the system name of the currently configured UART port.
    QString currentPort() const { return m_port.portName(); }
    QStringList portDisplayNames() const;
    QString portNameAt(int index) const;

    void refreshPorts();
    bool connectToPort(const QString &portName);
    void disconnectPort();
    bool writeData(const QByteArray &data);

signals:
    void portsChanged();
    void connectedChanged();
    void deviceIdReceived(const QString &deviceId);
    void sampleReceived(const SensorSample &sample);
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();

private:
    struct PortEntry {
        QString name;
        QString label;
    };

    void processLine(const QByteArray &line);

    QSerialPort m_port;
    QByteArray m_buffer;
    QVector<PortEntry> m_ports;
};

#endif
