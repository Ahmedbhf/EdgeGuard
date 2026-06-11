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
    Q_PROPERTY(double anomalyScore READ anomalyScore NOTIFY dataChanged)
    Q_PROPERTY(QString condition READ condition NOTIFY dataChanged)
    Q_PROPERTY(QString conditionTone READ conditionTone NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(double ambientTemp READ ambientTemp NOTIFY dataChanged)
    Q_PROPERTY(QVector<double> anomalyValues READ anomalyValues NOTIFY anomalyValuesChanged)
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
    Q_PROPERTY(QString stateSummary READ stateSummary NOTIFY stateChanged)
    Q_PROPERTY(QString operatingCondition READ operatingCondition NOTIFY stateChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString lastUpdateText READ lastUpdateText NOTIFY lastUpdateTextChanged)
    Q_PROPERTY(QString historyStatusText READ historyStatusText NOTIFY historyDataChanged)
    Q_PROPERTY(QVariantMap historyData READ historyData NOTIFY historyDataChanged)
    Q_PROPERTY(bool relearnAvailable READ relearnAvailable NOTIFY relearnAvailabilityChanged)
    Q_PROPERTY(int relearnCooldownSeconds READ relearnCooldownSeconds NOTIFY relearnCooldownChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    // Returns the latest anomaly score shown on the dashboard.
    double anomalyScore() const { return m_anomalyScore; }
    // Returns the current health condition label for the machine.
    QString condition() const { return m_condition; }
    // Returns the UI tone token that matches the current condition.
    QString conditionTone() const { return m_conditionTone; }
    // Returns the latest measured machine temperature.
    double temp() const { return m_temp; }
    // Returns the latest measured ambient temperature.
    double ambientTemp() const { return m_ambientTemp; }
    // Returns the rolling anomaly chart values.
    QVector<double> anomalyValues() const { return m_anomalyValues; }
    // Returns the rolling temperature chart values.
    QVector<double> temperatureValues() const { return m_temperatureValues; }
    // Returns the rolling X-axis acceleration values.
    QVector<double> xAxisValues() const { return m_xAxisValues; }
    // Returns the rolling Y-axis acceleration values.
    QVector<double> yAxisValues() const { return m_yAxisValues; }
    // Returns the rolling Z-axis acceleration values.
    QVector<double> zAxisValues() const { return m_zAxisValues; }
    // Reports whether the serial service currently owns an open port.
    bool connected() const { return m_serialService.connected(); }
    // Reports whether the connected hardware has sent a device identifier.
    bool deviceConnected() const { return !m_deviceId.isEmpty(); }
    // Returns display labels for the ports detected by the serial service.
    QStringList availablePorts() const { return m_serialService.portDisplayNames(); }
    // Returns the active port while connected, otherwise the user's selected port.
    QString currentPort() const { return connected() ? m_serialService.currentPort() : m_selectedPort; }
    // Returns the port currently selected in the setup UI.
    QString selectedPort() const { return m_selectedPort; }
    // Returns the machine type selected or inferred for this session.
    QString machineType() const { return m_machineType; }
    // Returns the latest device identifier reported by the hardware.
    QString deviceId() const { return m_deviceId; }
    // Returns the current machine state label from the device or score fallback.
    QString state() const { return m_state; }
    // Combines state and operating condition for compact UI display.
    QString stateSummary() const { return QStringLiteral("%1 · %2").arg(m_state, m_operatingCondition); }
    // Returns the current operating condition or detected fault class.
    QString operatingCondition() const { return m_operatingCondition; }
    // Returns the UI log buffer as newline-separated text.
    QString logText() const { return m_logs.join('\n'); }
    QString lastUpdateText() const;
    // Returns the status line for the currently loaded history chunk.
    QString historyStatusText() const { return m_historyStatusText; }
    // Returns chart-ready history data consumed by the History page.
    QVariantMap historyData() const { return m_historyData; }
    // Reports whether the relearn command can be sent right now.
    bool relearnAvailable() const { return connected() && m_relearnCooldownSeconds <= 0; }
    // Returns the remaining seconds before another relearn command is allowed.
    int relearnCooldownSeconds() const { return m_relearnCooldownSeconds; }

    Q_INVOKABLE void connectToPort(const QString &portName);
    Q_INVOKABLE void connectPreferredPort(const QString &preferredPort);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void disconnectAndReset();
    // Refreshes the serial port list exposed to QML.
    Q_INVOKABLE void refreshPorts() { m_serialService.refreshPorts(); }
    // Returns the raw system port name for a visible port row.
    Q_INVOKABLE QString portNameAt(int index) const { return m_serialService.portNameAt(index); }
    Q_INVOKABLE void setSelectedPort(const QString &portName);
    Q_INVOKABLE void resetDeviceIdentity();
    void setMachineType(const QString &machineType);
    Q_INVOKABLE void refreshHistoryData();
    Q_INVOKABLE void loadOlderHistoryChunk();
    Q_INVOKABLE void loadNewerHistoryChunk();
    Q_INVOKABLE bool exportHistoryCsv(const QUrl &fileUrl);
    Q_INVOKABLE void requestRelearn();

signals:
    void dataChanged();
    void anomalyValuesChanged();
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
    void relearnAvailabilityChanged();
    void relearnCooldownChanged();

private slots:
    void onSampleReceived(const SensorSample &sample);
    void flushLiveData();
    void updateLastUpdateText();
    void tickRelearnCooldown();

private:
    struct ParsedHistory {
        QVariantMap data;
        QString statusText;
    };

    void loadHistoryChunk();
    void processSample(const SensorSample &sample);
    void syncPorts();
    void syncConnection();
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
    QString m_operatingCondition = QStringLiteral("Motor On");
    QString m_historyStatusText = QStringLiteral("Loading the last 24 hours of stored history...");
    QVariantMap m_historyData;
    SensorSample m_latestSample;
    QVector<double> m_anomalyValues;
    QVector<double> m_temperatureValues;
    QVector<double> m_xAxisValues;
    QVector<double> m_yAxisValues;
    QVector<double> m_zAxisValues;
    QStringList m_logs;
    QTimer m_liveDataTimer;
    QTimer m_lastUpdateTimer;
    QTimer m_relearnCooldownTimer;
    bool m_pendingLiveRefresh = false;
    int m_windowSampleCount = 0;
    int m_storageSamplesSinceCleanup = 0;
    int m_historyChunkOffset = 0;
    qint64 m_lastStoredSampleMs = 0;
    double m_x = 0.0;
    double m_y = 0.0;
    double m_z = 0.0;
    double m_anomalyScore = 0.0;
    QString m_condition = QStringLiteral("CRITICAL");
    QString m_conditionTone = QStringLiteral("fault");
    double m_temp = 0.0;
    double m_ambientTemp = 0.0;
    double m_windowTempSum = 0.0;
    double m_windowAnomalySum = 0.0;
    double m_windowXSum = 0.0;
    double m_windowYSum = 0.0;
    double m_windowZSum = 0.0;
    int m_relearnCooldownSeconds = 0;

    static constexpr int MaxHistory = 300;
    static constexpr int LiveAggregationIntervalMs = 50;
    static constexpr int StorageIntervalMs = 250;
    static constexpr int StorageCleanupIntervalSamples = 20;
    static constexpr int MaxLogLines = 300;
    static constexpr int HistoryChunkSize = 1000;
    static constexpr int RelearnCooldownSeconds = 40;
};

#endif
