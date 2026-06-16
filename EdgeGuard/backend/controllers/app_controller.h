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
    Q_PROPERTY(double anomalyScore READ anomalyScore NOTIFY dataChanged) // Shown on Dashboard.qml in Anomaly Score GaugeCard (Line 180)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged) // Shown on Dashboard.qml in Temperature GaugeCard (Line 193)
    Q_PROPERTY(QVector<double> anomalyValues READ anomalyValues NOTIFY anomalyValuesChanged) // Used by the live Score vs Time LiveTrendChart in Dashboard.qml (Line 130)
    Q_PROPERTY(QVector<double> xAxisValues READ xAxisValues NOTIFY xAxisValuesChanged) // Used by the live Vibration LiveTrendChart in Dashboard.qml when axis X is selected
    Q_PROPERTY(QVector<double> yAxisValues READ yAxisValues NOTIFY yAxisValuesChanged) // Used by the live Vibration LiveTrendChart in Dashboard.qml when axis Y is selected
    Q_PROPERTY(QVector<double> zAxisValues READ zAxisValues NOTIFY zAxisValuesChanged) // Used by the live Vibration LiveTrendChart in Dashboard.qml when axis Z is selected
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged) // Toggles Connect/Disconnect labels in DashboardHeaderBar.qml (Line 59)
    Q_PROPERTY(bool deviceConnected READ deviceConnected NOTIFY deviceIdChanged) // Binding source for 'deviceReady' in SetupPage.qml to enable 'Continue' button (Line 15)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged) // Exposes list of COM ports to SetupPage
    Q_PROPERTY(QString machineType READ machineType WRITE setMachineType NOTIFY machineTypeChanged) // Selected in SetupPage grid (Line 127) and shown in DashboardHeaderBar.qml (Line 40)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceIdChanged) // Shown on SetupPage.qml (Line 184) and DashboardHeaderBar.qml (Line 41)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged) // Dictates visual states/alarms (e.g. NORMAL/WARNING/CRITICAL) on the Dashboard
    Q_PROPERTY(QString operatingCondition READ operatingCondition NOTIFY stateChanged) // Displays details of the machine state (e.g., motor off, friction) on Dashboard.qml (Line 100)
    Q_PROPERTY(QString historyStatusText READ historyStatusText NOTIFY historyDataChanged) // Shows paged database index range in HistoryPage.qml (Line 100)
    Q_PROPERTY(QVariantMap historyData READ historyData NOTIFY historyDataChanged) // Source of historical points and FFT spectrum for charts in HistoryPage.qml (Line 112)
    Q_PROPERTY(bool relearnAvailable READ relearnAvailable NOTIFY relearnAvailabilityChanged) // Enable/Disable state of the 'Relearn' button in DashboardHeaderBar.qml (Line 68)
    Q_PROPERTY(int relearnCooldownSeconds READ relearnCooldownSeconds NOTIFY relearnCooldownChanged) // Displays count-down timer next to Relearn text in DashboardHeaderBar.qml (Line 66)

public:
    explicit AppController(QObject *parent = nullptr);

    // Returns the latest anomaly score shown on the dashboard.
    double anomalyScore() const { return m_anomalyScore; }
    // Returns the latest measured machine temperature.
    double temp() const { return m_temp; }
    // Returns the rolling anomaly chart values.
    QVector<double> anomalyValues() const { return m_anomalyValues; }
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
    // Returns the machine type selected or inferred for this session.
    QString machineType() const { return m_machineType; }
    // Returns the latest device identifier reported by the hardware.
    QString deviceId() const { return m_deviceId; }
    // Returns the current machine state label from the device or score fallback.
    QString state() const { return m_state; }
    // Returns the current operating condition or detected fault class.
    QString operatingCondition() const { return m_operatingCondition; }
    // Returns the status line for the currently loaded history chunk.
    QString historyStatusText() const { return m_historyStatusText; }
    // Returns chart-ready history data consumed by the History page.
    QVariantMap historyData() const { return m_historyData; }
    // Reports whether the relearn command can be sent right now.
    bool relearnAvailable() const { return connected() && m_relearnCooldownSeconds <= 0; }
    // Returns the remaining seconds before another relearn command is allowed.
    int relearnCooldownSeconds() const { return m_relearnCooldownSeconds; }

    // Connects to a specific COM/UART port. Used internally.
    Q_INVOKABLE void connectToPort(const QString &portName);
    // Called by SetupPage.qml "Connect Device" ControlButton (Line 113)
    Q_INVOKABLE void connectPreferredPort(const QString &preferredPort);
    // Closes UART port. Used internally.
    Q_INVOKABLE void disconnectPort();
    // Called by DashboardHeaderBar.qml "Disconnect" ControlButton (Line 61)
    Q_INVOKABLE void disconnectAndReset();
    // Rescans available system COM/TTY ports.
    Q_INVOKABLE void refreshPorts() { m_serialService.refreshPorts(); }
    // Helper to resolve the system port name behind the selected list view row.
    Q_INVOKABLE QString portNameAt(int index) const { return m_serialService.portNameAt(index); }
    // Stores the setup page's selected port. Used internally.
    Q_INVOKABLE void setSelectedPort(const QString &portName);
    // Resets identity state. Used internally.
    Q_INVOKABLE void resetDeviceIdentity();
    // Sets the machine type. Used internally.
    void setMachineType(const QString &machineType);
    // Called by "Refresh" ControlButton in HistoryPage.qml (Line 78)
    Q_INVOKABLE void refreshHistoryData();
    // Called by "Older" ControlButton in HistoryPage.qml (Line 84) to page backward
    Q_INVOKABLE void loadOlderHistoryChunk();
    // Called by "Newer" ControlButton in HistoryPage.qml (Line 90) to page forward
    Q_INVOKABLE void loadNewerHistoryChunk();
    // Called by "Export CSV" ControlButton in HistoryPage.qml (Line 95)
    Q_INVOKABLE bool exportHistoryCsv(const QUrl &fileUrl);
    // Called by "Relearn" ControlButton in DashboardHeaderBar.qml (Line 69)
    Q_INVOKABLE void requestRelearn();

signals:
    void dataChanged();
    void anomalyValuesChanged();
    void xAxisValuesChanged();
    void yAxisValuesChanged();
    void zAxisValuesChanged();
    void connectedChanged();
    void availablePortsChanged();
    void machineTypeChanged();
    void deviceIdChanged();
    void stateChanged();
    void historyDataChanged();
    void relearnAvailabilityChanged();
    void relearnCooldownChanged();

private slots:
    void onSampleReceived(const SensorSample &sample);
    void flushLiveData();
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
    QVector<double> m_xAxisValues;
    QVector<double> m_yAxisValues;
    QVector<double> m_zAxisValues;
    QTimer m_liveDataTimer;
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
    double m_temp = 0.0;
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
    static constexpr int HistoryChunkSize = 1000;
    static constexpr int RelearnCooldownSeconds = 40;
};

#endif
