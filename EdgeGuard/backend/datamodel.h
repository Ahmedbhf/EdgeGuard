#ifndef DATAMODEL_H
#define DATAMODEL_H

#include "serialmanager.h"

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <memory>

class DataStorage;

class DataModel : public QObject
{
    Q_OBJECT
    // These properties expose live values to QML so the UI can bind directly to backend state.
    Q_PROPERTY(double rms READ rms NOTIFY dataChanged)
    Q_PROPERTY(double anomalyScore READ anomalyScore NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(double ambientTemp READ ambientTemp NOTIFY dataChanged)
    Q_PROPERTY(QVector<double> vibrationValues READ vibrationValues NOTIFY vibrationValuesChanged)
    Q_PROPERTY(QVector<double> temperatureValues READ temperatureValues NOTIFY temperatureValuesChanged)
    Q_PROPERTY(QVector<double> xAxisValues READ xAxisValues NOTIFY xAxisValuesChanged)
    Q_PROPERTY(QVector<double> yAxisValues READ yAxisValues NOTIFY yAxisValuesChanged)
    Q_PROPERTY(QVector<double> zAxisValues READ zAxisValues NOTIFY zAxisValuesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(QString machineType READ machineType WRITE setMachineType NOTIFY machineTypeChanged)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)

public:
    explicit DataModel(QObject *parent = nullptr);
    ~DataModel() override;

    double rms() const { return m_rms; }
    double anomalyScore() const { return m_anomalyScore; }
    double temp() const { return m_temp; }
    double ambientTemp() const { return m_ambientTemp; }
    QVector<double> vibrationValues() const { return m_vibration; }
    QVector<double> temperatureValues() const { return m_temperature; }
    QVector<double> xAxisValues() const { return m_xHistory; }
    QVector<double> yAxisValues() const { return m_yHistory; }
    QVector<double> zAxisValues() const { return m_zHistory; }
    bool connected() const { return m_serial->connected(); }
    QStringList availablePorts() const { return m_serial->portDisplayNames(); }
    QString currentPort() const { return connected() ? m_serial->currentPort() : m_selectedPort; }
    QString selectedPort() const { return m_selectedPort; }
    QString machineType() const { return m_machineType; }
    QString deviceId() const { return m_deviceId; }
    QString state() const { return m_state; }
    QString logText() const { return m_logs.join('\n'); }

    Q_INVOKABLE void connectToPort(const QString &portName);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void refreshPorts() { m_serial->refreshPorts(); }
    Q_INVOKABLE void setSelectedPort(const QString &portName);
    void setMachineType(const QString &machineType);
    void setDeviceId(const QString &deviceId);
    Q_INVOKABLE QString portNameAt(int index) const { return m_serial->portNameAt(index); }
    Q_INVOKABLE QString loadLast24hCsv() const;
    Q_INVOKABLE bool exportHistoryCsv(const QUrl &fileUrl);

signals:
    void dataChanged();
    void vibrationValuesChanged();
    void temperatureValuesChanged();
    void xAxisValuesChanged();
    void yAxisValuesChanged();
    void zAxisValuesChanged();
    void connectedChanged();
    void availablePortsChanged();
    void selectedPortChanged();
    void machineTypeChanged();
    void deviceIdChanged();
    void stateChanged();
    void logTextChanged();

    private slots:
    // Raw serial packets arrive here first, then we turn them into app state.
    void onPacketReceived(double anomalyScore, double x, double y, double z, double temp, double ambientTemp, const QString &stateText);
    // This runs on a timer to push batched samples into the UI at a steady pace.
    void flushUiSamples();

    private:
    // Updates all calculated values for one incoming sample.
    void processSample(double anomalyScore, double x, double y, double z, double temp, double ambientTemp, const QString &stateText);
    // Keeps the selected serial port in sync with the list from the OS.
    void syncPorts();
    // Handles UI state changes when the serial link connects or disconnects.
    void syncConnection();
    // Recomputes summary values shown on the dashboard.
    void updateMetrics();
    // Persists a downsampled snapshot into the rolling 24h CSV store.
    void storeHistoryPoint(double anomalyScore, double x, double y, double z, double temp);
    // Adds one line to the on-screen event log.
    void appendLog(const QString &text);

    SerialManager *m_serial = nullptr;
    QString m_selectedPort;
    QString m_machineType;
    QString m_deviceId;
    QString m_state = QStringLiteral("OK");
    double m_x = 0.0, m_y = 0.0, m_z = 0.0, m_rms = 0.0, m_anomalyScore = 0.0, m_temp = 0.0, m_ambientTemp = 0.0;
    QVector<double> m_vibration;
    QVector<double> m_temperature;
    QVector<double> m_xHistory;
    QVector<double> m_yHistory;
    QVector<double> m_zHistory;
    QStringList m_logs;
    QTimer m_uiSampleTimer;
    bool m_pendingUiRefresh = false;
    int m_windowSampleCount = 0;
    double m_windowRmsSquareSum = 0.0;
    double m_windowTempSum = 0.0;
    double m_windowAnomalySum = 0.0;
    double m_windowXSum = 0.0;
    double m_windowYSum = 0.0;
    double m_windowZSum = 0.0;
    int m_storageSamplesSinceCleanup = 0;
    qint64 m_lastStoredSampleMs = 0;
    std::unique_ptr<DataStorage> m_storage;

    // Small fixed limits keep charts and logs responsive even during long runs.
    static constexpr int MaxHistory = 300;
    static constexpr int UiAggregationIntervalMs = 50;
    static constexpr int StorageIntervalMs = 250;
    static constexpr int StorageCleanupIntervalSamples = 20;
    static constexpr int MaxLogLines = 300;
};

#endif
