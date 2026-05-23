#include "app_controller.h"

#include <QDateTime>

// Wires services, timers, and startup refresh. Other AppController behavior lives in focused files.
AppController::AppController(QObject *parent) : QObject(parent)
{
    connect(&m_serialService, &SerialService::deviceIdReceived, this, [this](const QString &deviceId) {
        const QString trimmedDeviceId = deviceId.trimmed();
        if (!trimmedDeviceId.isEmpty() && m_deviceId != trimmedDeviceId) {
            m_deviceId = trimmedDeviceId;
            emit deviceIdChanged();
        }

        if (m_machineType.isEmpty())
            setMachineType(QStringLiteral("DC Motor"));
    });
    connect(&m_serialService, &SerialService::sampleReceived, this, &AppController::onSampleReceived);
    connect(&m_serialService, &SerialService::errorOccurred, this, [this](const QString &text) { appendLog(text); });
    connect(&m_serialService, &SerialService::portsChanged, this, &AppController::syncPorts);
    connect(&m_serialService, &SerialService::connectedChanged, this, &AppController::syncConnection);

    m_liveDataTimer.setInterval(LiveAggregationIntervalMs);
    connect(&m_liveDataTimer, &QTimer::timeout, this, &AppController::flushLiveData);
    m_liveDataTimer.start();

    m_lastUpdateTimer.setInterval(1000);
    connect(&m_lastUpdateTimer, &QTimer::timeout, this, &AppController::updateLastUpdateText);
    m_lastUpdateTimer.start();

    m_relearnCooldownTimer.setInterval(1000);
    connect(&m_relearnCooldownTimer, &QTimer::timeout, this, &AppController::tickRelearnCooldown);

    m_lastStoredSampleMs = QDateTime::currentMSecsSinceEpoch();
    refreshPorts();
    refreshHistoryData();
    appendLog(QStringLiteral("Ready."));
}

// Builds the "Just now" / "N s ago" text used beside live sensor data.
QString AppController::lastUpdateText() const
{
    if (!m_latestSample.timestampUtc.isValid())
        return QStringLiteral("Waiting for data");

    const qint64 seconds = m_latestSample.timestampUtc.secsTo(QDateTime::currentDateTimeUtc());
    if (seconds <= 1)
        return QStringLiteral("Just now");

    return QStringLiteral("%1 s ago").arg(seconds);
}
