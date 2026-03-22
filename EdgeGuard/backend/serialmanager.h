#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QVector>

class SerialManager : public QObject
{
    Q_OBJECT
    // QML reads these properties to show connection status and available serial devices.
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(QStringList portDisplayNames READ portDisplayNames NOTIFY portsChanged)

public:
    explicit SerialManager(QObject *parent = nullptr);

    bool connected() const { return m_port.isOpen(); }
    QString currentPort() const { return m_port.portName(); }
    QStringList portDisplayNames() const;
    QString portNameAt(int index) const;

    void refreshPorts();
    bool connectToPort(const QString &portName);
    void disconnectPort();

signals:
    void portsChanged();
    void connectedChanged();
    void deviceIdReceived(const QString &deviceId);
    void packetReceived(double anomalyScore, double x, double y, double z, double temp, const QString &stateText);
    void errorOccurred(const QString &message);

private slots:
    // Serial data may arrive in chunks, so we buffer it until we get full lines.
    void onReadyRead();

private:
    struct PortEntry { QString name; QString label; };

    // Parses one complete text line from the device protocol.
    void processLine(const QByteArray &line);

    QSerialPort m_port;
    QByteArray m_buffer;
    QVector<PortEntry> m_ports;
};

#endif
