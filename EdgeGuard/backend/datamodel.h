#ifndef DATAMODEL_H
#define DATAMODEL_H

#include "serialmanager.h"

#include <QFile>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>

class DataModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double rms READ rms NOTIFY dataChanged)
    Q_PROPERTY(double anomalyScore READ anomalyScore NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(QVector<double> vibrationValues READ vibrationValues NOTIFY vibrationValuesChanged)
    Q_PROPERTY(QVector<double> temperatureValues READ temperatureValues NOTIFY temperatureValuesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(QString machineType READ machineType WRITE setMachineType NOTIFY machineTypeChanged)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString csvFilePath READ csvFilePath NOTIFY csvFilePathChanged)
    Q_PROPERTY(bool loggingEnabled READ loggingEnabled NOTIFY loggingEnabledChanged)

public:
    explicit DataModel(QObject *parent = nullptr);
    ~DataModel() override;

    double rms() const { return m_rms; }
    double anomalyScore() const { return m_anomalyScore; }
    double temp() const { return m_temp; }
    QVector<double> vibrationValues() const { return m_vibration; }
    QVector<double> temperatureValues() const { return m_temperature; }
    bool connected() const { return m_serial->connected(); }
    QStringList availablePorts() const { return m_serial->portDisplayNames(); }
    QString currentPort() const { return connected() ? m_serial->currentPort() : m_selectedPort; }
    QString selectedPort() const { return m_selectedPort; }
    QString machineType() const { return m_machineType; }
    QString deviceId() const { return m_deviceId; }
    QString state() const { return m_state; }
    QString logText() const { return m_logs.join('\n'); }
    QString csvFilePath() const { return m_csvPath; }
    bool loggingEnabled() const { return m_loggingEnabled; }

    Q_INVOKABLE void connectToPort(const QString &portName);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void toggleConnection();
    Q_INVOKABLE void refreshPorts() { m_serial->refreshPorts(); }
    Q_INVOKABLE void setSelectedPort(const QString &portName);
    void setMachineType(const QString &machineType);
    void setDeviceId(const QString &deviceId);
    Q_INVOKABLE QString portNameAt(int index) const { return m_serial->portNameAt(index); }
    Q_INVOKABLE void openCsvFile();
    Q_INVOKABLE void startLogging();
    Q_INVOKABLE void stopLogging();
    Q_INVOKABLE QString readTextFile(const QUrl &fileUrl) const;
    Q_INVOKABLE QString readTextFileLimited(const QUrl &fileUrl, int maxLines) const;

signals:
    void dataChanged();
    void vibrationValuesChanged();
    void temperatureValuesChanged();
    void connectedChanged();
    void availablePortsChanged();
    void selectedPortChanged();
    void machineTypeChanged();
    void deviceIdChanged();
    void stateChanged();
    void logTextChanged();
    void csvFilePathChanged();
    void loggingEnabledChanged();

private slots:
    void onPacketReceived(double anomalyScore, double x, double y, double z, double temp, const QString &stateText);
    void flushUiSamples();

private:
    void processSample(double anomalyScore, double x, double y, double z, double temp, const QString &stateText);
    void syncPorts();
    void syncConnection();
    void updateMetrics();
    void appendLog(const QString &text);
    void startCsv();
    void stopCsv();
    void writeCsv();

    SerialManager *m_serial = nullptr;
    QString m_selectedPort;
    QString m_machineType;
    QString m_deviceId;
    QString m_state = QStringLiteral("OK");
    QString m_csvPath;
    double m_x = 0.0, m_y = 0.0, m_z = 0.0, m_rms = 0.0, m_anomalyScore = 0.0, m_temp = 0.0;
    QVector<double> m_vibration;
    QVector<double> m_temperature;
    QStringList m_logs;
    QFile m_csv;
    QTimer m_uiSampleTimer;
    bool m_loggingEnabled = false;
    bool m_pendingUiRefresh = false;
    int m_windowSampleCount = 0;
    int m_csvWritesSinceFlush = 0;
    double m_windowRmsSquareSum = 0.0;
    double m_windowTempSum = 0.0;

    static constexpr int MaxHistory = 300;
    static constexpr int UiAggregationIntervalMs = 50;
    static constexpr int CsvFlushInterval = 20;
    static constexpr int MaxLogLines = 300;
};

#endif
