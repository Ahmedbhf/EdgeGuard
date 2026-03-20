#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTimer>
#include <QVector>

class SerialManager : public QObject
{
    Q_OBJECT
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
    void packetReceived(double x, double y, double z, double temp, const QString &stateText, int status);
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);
    void onTimeout();

private:
    struct PortEntry { QString name; QString label; };

    void processLine(const QByteArray &line);

    QSerialPort m_port;
    QByteArray m_buffer;
    QTimer m_timeout;
    QVector<PortEntry> m_ports;
};

#endif
