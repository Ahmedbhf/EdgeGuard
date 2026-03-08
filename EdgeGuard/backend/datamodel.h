#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QObject>
#include <QVector>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

class DataModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double rms READ rms NOTIFY dataChanged)
    Q_PROPERTY(double peak2peak READ peak2peak NOTIFY dataChanged)
    Q_PROPERTY(double variance READ variance NOTIFY dataChanged)
    Q_PROPERTY(double crestFactor READ crestFactor NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(double tempSlope READ tempSlope NOTIFY dataChanged)
    Q_PROPERTY(double zScore READ zScore NOTIFY dataChanged)

    // Chart history
    Q_PROPERTY(QVector<double> vibrationValues READ vibrationValues NOTIFY vibrationValuesChanged)
    Q_PROPERTY(QVector<double> temperatureValues READ temperatureValues NOTIFY temperatureValuesChanged)

    // Serial port state
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(int motorId READ motorId NOTIFY dataChanged)
    Q_PROPERTY(int motorStatus READ motorStatus NOTIFY dataChanged)

    // Simulation mode flag
    Q_PROPERTY(bool simulationMode READ simulationMode NOTIFY simulationModeChanged)

public:
    explicit DataModel(QObject *parent = nullptr);

    double rms() const;
    double peak2peak() const;
    double variance() const;
    double crestFactor() const;
    double temp() const;
    double tempSlope() const;
    double zScore() const;

    QVector<double> vibrationValues() const { return m_vibrationValues; }
    QVector<double> temperatureValues() const { return m_temperatureValues; }

    bool connected() const;
    QStringList availablePorts() const;
    QString currentPort() const;
    int motorId() const;
    int motorStatus() const;
    bool simulationMode() const;

    Q_INVOKABLE void connectToPort(const QString &portName);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void startSimulation();
    Q_INVOKABLE void stopSimulation();

signals:
    void dataChanged();
    void vibrationValuesChanged();
    void temperatureValuesChanged();
    void connectedChanged();
    void availablePortsChanged();
    void serialError(const QString &message);
    void simulationModeChanged();

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onTimeoutTick();
    void onSimulationTick();

private:
    void processLine(const QByteArray &line);
    void compute();

    // Serial
    QSerialPort *m_serial;
    QByteArray m_readBuffer;
    QTimer *m_timeoutTimer;
    QTimer *m_uiTimer;

    // Simulation
    QTimer *m_simTimer;
    bool m_simulationMode;
    double m_simTime;

    // Parsed fields
    int m_motorId;
    int m_motorStatus;

    // Accel / computed
    double m_x;
    double m_y;
    double m_z;

    double m_rms;
    double m_peak2peak;
    double m_variance;
    double m_crestFactor;
    double m_temp;
    double m_tempSlope;

    double m_lastTemp;
    double m_meanRms;
    double m_stdRms;
    double m_zScore;

    // Signal history
    QVector<double> m_vibrationValues;
    QVector<double> m_temperatureValues;
    const int MAX_HISTORY = 2000;
};

#endif
