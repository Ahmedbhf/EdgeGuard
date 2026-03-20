#ifndef DATAMODEL_H
#define DATAMODEL_H

#include "serialmanager.h"

#include <QFile>
#include <QObject>
#include <QStringList>
#include <QVector>

class DataModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double rms READ rms NOTIFY dataChanged)
    Q_PROPERTY(double peak2peak READ peak2peak NOTIFY dataChanged)
    Q_PROPERTY(double variance READ variance NOTIFY dataChanged)
    Q_PROPERTY(double crestFactor READ crestFactor NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(double tempSlope READ tempSlope NOTIFY dataChanged)
    Q_PROPERTY(QVector<double> vibrationValues READ vibrationValues NOTIFY vibrationValuesChanged)
    Q_PROPERTY(QVector<double> temperatureValues READ temperatureValues NOTIFY temperatureValuesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString csvFilePath READ csvFilePath NOTIFY csvFilePathChanged)

public:
    explicit DataModel(QObject *parent = nullptr);
    ~DataModel() override;

    double rms() const { return m_rms; }
    double peak2peak() const { return m_peak2peak; }
    double variance() const { return m_variance; }
    double crestFactor() const { return m_crestFactor; }
    double temp() const { return m_temp; }
    double tempSlope() const { return m_tempSlope; }
    QVector<double> vibrationValues() const { return m_vibration; }
    QVector<double> temperatureValues() const { return m_temperature; }
    bool connected() const { return m_serial->connected(); }
    QStringList availablePorts() const { return m_serial->portDisplayNames(); }
    QString currentPort() const { return connected() ? m_serial->currentPort() : m_selectedPort; }
    QString selectedPort() const { return m_selectedPort; }
    QString state() const { return m_state; }
    QString logText() const { return m_logs.join('\n'); }
    QString csvFilePath() const { return m_csvPath; }

    Q_INVOKABLE void connectToPort(const QString &portName);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void toggleConnection();
    Q_INVOKABLE void refreshPorts() { m_serial->refreshPorts(); }
    Q_INVOKABLE void setSelectedPort(const QString &portName);
    Q_INVOKABLE QString portNameAt(int index) const { return m_serial->portNameAt(index); }
    Q_INVOKABLE void openCsvFile();

signals:
    void dataChanged();
    void vibrationValuesChanged();
    void temperatureValuesChanged();
    void connectedChanged();
    void availablePortsChanged();
    void selectedPortChanged();
    void stateChanged();
    void logTextChanged();
    void csvFilePathChanged();

private slots:
    void onPacketReceived(double x, double y, double z, double temp, const QString &, int status);

private:
    void syncPorts();
    void syncConnection();
    void updateMetrics();
    void appendHistory();
    void appendLog(const QString &text);
    void startCsv();
    void stopCsv();
    void writeCsv();

    SerialManager *m_serial = nullptr;
    QString m_selectedPort;
    QString m_state = QStringLiteral("OK");
    QString m_csvPath;
    double m_x = 0.0, m_y = 0.0, m_z = 0.0, m_rms = 0.0, m_peak2peak = 0.0;
    double m_variance = 0.0, m_crestFactor = 0.0, m_temp = 0.0, m_tempSlope = 0.0;
    double m_lastTemp = 0.0;
    QVector<double> m_vibration;
    QVector<double> m_temperature;
    QStringList m_logs;
    QFile m_csv;

    static constexpr int MaxHistory = 240;
    static constexpr int MaxLogLines = 300;
};

#endif
