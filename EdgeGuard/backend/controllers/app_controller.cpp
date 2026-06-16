#include "app_controller.h"

#include <QDateTime>

// =========================================================================
// AppController Constructor
// =========================================================================
// Wires internal services, registers signal-to-slot connections, sets up
// active polling/refresh timers, and runs startup database/port initialization.
AppController::AppController(QObject *parent) : QObject(parent)
{
    // ---------------------------------------------------------------------
    // 1. SerialService Signal Connections
    // ---------------------------------------------------------------------

    // Signal: deviceIdReceived
    // Triggered when: The microcontroller sends its unique identifier code over UART.
    // Action: Saves the ID, reloads historical database records specifically for this machine,
    //         and defaults the machine type selection to "DC Motor" if it hasn't been set yet.
    connect(&m_serialService, &SerialService::deviceIdReceived, this, [this](const QString &deviceId) {
        const QString trimmedDeviceId = deviceId.trimmed();
        if (!trimmedDeviceId.isEmpty() && m_deviceId != trimmedDeviceId) {
            m_deviceId = trimmedDeviceId;
            emit deviceIdChanged();
            refreshHistoryData();
        }

        if (m_machineType.isEmpty())
            setMachineType(QStringLiteral("DC Motor"));
    });

    // Signal: sampleReceived
    // Triggered when: A telemetry line (acceleration, temperature, anomaly score) is parsed from UART.
    // Action: Routes the raw packet to AppController::onSampleReceived() in app_controller_live.cpp.
    connect(&m_serialService, &SerialService::sampleReceived, this, &AppController::onSampleReceived);

    // Signal: errorOccurred
    // Triggered when: Serial port reading or connection fails.
    // Action: Formats the error and prints it to the debugging output log.
    connect(&m_serialService, &SerialService::errorOccurred, this, [this](const QString &text) { appendLog(text); });

    // Signal: portsChanged
    // Triggered when: A serial port is plugged or unplugged from the host machine.
    // Action: Runs AppController::syncPorts() to refresh the available COM ports selection.
    connect(&m_serialService, &SerialService::portsChanged, this, &AppController::syncPorts);

    // Signal: connectedChanged
    // Triggered when: Serial port status transitions between Open (Connected) and Closed (Disconnected).
    // Action: Invokes AppController::syncConnection() to toggle QML buttons and state displays.
    connect(&m_serialService, &SerialService::connectedChanged, this, &AppController::syncConnection);

    // ---------------------------------------------------------------------
    // 2. Timer Setup
    // ---------------------------------------------------------------------

    // Live Aggregation Timer:
    // Role: Ticks every 50ms to aggregate and average raw high-frequency telemetry data
    //       points before pushing them to live QML trend graphs, preventing interface lag.
    m_liveDataTimer.setInterval(LiveAggregationIntervalMs);
    connect(&m_liveDataTimer, &QTimer::timeout, this, &AppController::flushLiveData);
    m_liveDataTimer.start();

    // Relearn Cooldown Timer:
    // Role: Ticks every 1000ms (1 second) to count down the 40-second relearn command lockout
    //       period and update the countdown text displayed inside the Relearn button.
    m_relearnCooldownTimer.setInterval(1000);
    connect(&m_relearnCooldownTimer, &QTimer::timeout, this, &AppController::tickRelearnCooldown);

    // ---------------------------------------------------------------------
    // 3. Startup Initialization
    // ---------------------------------------------------------------------
    m_lastStoredSampleMs = QDateTime::currentMSecsSinceEpoch();
    refreshPorts();         // Scans OS serial registry for plugged devices on startup.
    refreshHistoryData();   // Queries SQLite to load the last 24h history chart points.
    appendLog(QStringLiteral("Ready."));
}


