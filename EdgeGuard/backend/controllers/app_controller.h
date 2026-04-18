#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "../models/sensor_sample.h"
#include "../services/data_storage_service.h"
#include "../services/serial_service.h"
#include "../../core/services/signal_processing_service.h"

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QVector>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double rms READ rms NOTIFY dataChanged)
    Q_PROPERTY(double anomalyScore READ anomalyScore NOTIFY dataChanged)
    Q_PROPERTY(QString condition READ condition NOTIFY dataChanged)
    Q_PROPERTY(QString conditionTone READ conditionTone NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(double ambientTemp READ ambientTemp NOTIFY dataChanged)
    Q_PROPERTY(QVector<double> anomalyValues READ anomalyValues NOTIFY anomalyValuesChanged)
    Q_PROPERTY(QVector<double> vibrationValues READ vibrationValues NOTIFY vibrationValuesChanged)
    Q_PROPERTY(QVector<double> temperatureValues READ temperatureValues NOTIFY temperatureValuesChanged)
    Q_PROPERTY(QVector<double> xAxisValues READ xAxisValues NOTIFY xAxisValuesChanged)
    Q_PROPERTY(QVector<double> yAxisValues READ yAxisValues NOTIFY yAxisValuesChanged)
    Q_PROPERTY(QVector<double> zAxisValues READ zAxisValues NOTIFY zAxisValuesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool deviceConnected READ deviceConnected NOTIFY deviceIdChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString currentPort READ currentPort NOTIFY connectedChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(QString machineType READ machineType WRITE setMachineType NOTIFY machineTypeChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString lastUpdateText READ lastUpdateText NOTIFY lastUpdateTextChanged)
    Q_PROPERTY(QString historyStatusText READ historyStatusText NOTIFY historyDataChanged)
    Q_PROPERTY(QVariantMap historyData READ historyData NOTIFY historyDataChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    double rms() const { return m_rms; }
    double anomalyScore() const { return m_anomalyScore; }
    QString condition() const { return m_condition; }
    QString conditionTone() const { return m_conditionTone; }
    double temp() const { return m_temp; }
    double ambientTemp() const { return m_ambientTemp; }
    QVector<double> anomalyValues() const { return m_anomalyValues; }
    QVector<double> vibrationValues() const { return m_vibrationValues; }
    QVector<double> temperatureValues() const { return m_temperatureValues; }
    QVector<double> xAxisValues() const { return m_xAxisValues; }
    QVector<double> yAxisValues() const { return m_yAxisValues; }
    QVector<double> zAxisValues() const { return m_zAxisValues; }
    bool connected() const { return m_serialService.connected(); }
    bool deviceConnected() const { return !m_deviceId.isEmpty(); }
    QStringList availablePorts() const { return m_serialService.portDisplayNames(); }
    QString currentPort() const { return connected() ? m_serialService.currentPort() : m_selectedPort; }
    QString selectedPort() const { return m_selectedPort; }
    QString machineType() const { return m_machineType; }
    QString deviceId() const { return m_deviceId; }
    QString state() const { return m_state; }
    QString logText() const { return m_logs.join('\n'); }
    QString lastUpdateText() const;
    QString historyStatusText() const { return m_historyStatusText; }
    QVariantMap historyData() const { return m_historyData; }

    Q_INVOKABLE void connectToPort(const QString &portName);
    Q_INVOKABLE void connectPreferredPort(const QString &preferredPort);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void disconnectAndReset();
    Q_INVOKABLE void refreshPorts() { m_serialService.refreshPorts(); }
    Q_INVOKABLE QString portNameAt(int index) const { return m_serialService.portNameAt(index); }
    Q_INVOKABLE void setSelectedPort(const QString &portName);
    Q_INVOKABLE void resetDeviceIdentity();
    void setMachineType(const QString &machineType);
    Q_INVOKABLE void refreshHistoryData();
    Q_INVOKABLE void loadOlderHistoryChunk();
    Q_INVOKABLE void loadNewerHistoryChunk();
    Q_INVOKABLE bool exportHistoryCsv(const QUrl &fileUrl);

signals:
    void dataChanged();
    void anomalyValuesChanged();
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
    void lastUpdateTextChanged();
    void historyDataChanged();

private slots:
    void onSampleReceived(const SensorSample &sample);
    void flushLiveData();
    void updateLastUpdateText();

private:
    struct ParsedHistory {
        QVariantMap data;
        QString statusText;
    };

    void loadHistoryChunk();
    void processSample(const SensorSample &sample);
    void syncPorts();
    void syncConnection();
    void updateLiveMetrics();
    void updateCondition();
    void appendLog(const QString &text);
    void storeHistorySample(double anomalyScore, double x, double y, double z, double temp);
    ParsedHistory parseHistorySamples(const QVector<SensorSample> &samples) const;
    void appendHistoryFftData(QVariantMap &data,
                              const QVector<double> &accelXValues,
                              const QVector<double> &accelYValues,
                              const QVector<double> &accelZValues,
                              const QVector<qint64> &timestampsMs) const;
    void updateHistoryData(const ParsedHistory &parsedHistory);
    void clearHistoryData(const QString &statusText);
    static void appendValue(QVector<double> &values, double value, int maxHistory);
    static QString toneForCondition(const QString &condition);

    SerialService m_serialService;
    DataStorageService m_storageService;
    SignalProcessingService m_signalProcessingService;
    QString m_selectedPort;
    QString m_machineType;
    QString m_deviceId;
    QString m_state = QStringLiteral("NORMAL");
    QString m_historyStatusText = QStringLiteral("Loading the last 24 hours of stored history...");
    QVariantMap m_historyData;
    SensorSample m_latestSample;
    QVector<double> m_anomalyValues;
    QVector<double> m_vibrationValues;
    QVector<double> m_temperatureValues;
    QVector<double> m_xAxisValues;
    QVector<double> m_yAxisValues;
    QVector<double> m_zAxisValues;
    QStringList m_logs;
    QTimer m_liveDataTimer;
    QTimer m_lastUpdateTimer;
    bool m_pendingLiveRefresh = false;
    int m_windowSampleCount = 0;
    int m_storageSamplesSinceCleanup = 0;
    int m_historyChunkOffset = 0;
    qint64 m_lastStoredSampleMs = 0;
    double m_x = 0.0;
    double m_y = 0.0;
    double m_z = 0.0;
    double m_rms = 0.0;
    double m_anomalyScore = 0.0;
    QString m_condition = QStringLiteral("FAULT");
    QString m_conditionTone = QStringLiteral("fault");
    double m_temp = 0.0;
    double m_ambientTemp = 0.0;
    double m_windowRmsSquareSum = 0.0;
    double m_windowTempSum = 0.0;
    double m_windowAnomalySum = 0.0;
    double m_windowXSum = 0.0;
    double m_windowYSum = 0.0;
    double m_windowZSum = 0.0;

    static constexpr int MaxHistory = 300;
    static constexpr int LiveAggregationIntervalMs = 50;
    static constexpr int StorageIntervalMs = 250;
    static constexpr int StorageCleanupIntervalSamples = 20;
    static constexpr int MaxLogLines = 300;
    static constexpr int HistoryChunkSize = 1000;
};

#endif
