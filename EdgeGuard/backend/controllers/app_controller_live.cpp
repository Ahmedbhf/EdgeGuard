#include "app_controller.h"

// Live sample path: raw UART samples become current values, chart buffers, and score condition.
// Entry point for serial samples; keeps the signal handler tiny and focused.
// =========================================================================
// 1. onSampleReceived (Internal Slot)
// =========================================================================
// Triggered by: SerialService::sampleReceived signal (Line 19 of app_controller.cpp)
//               whenever a valid telemetry line is parsed from UART/serial.
// Role: Serves as the immediate receiver for incoming real-time sensor packets,
//       passing the parsed SensorSample to the processSample method.
void AppController::onSampleReceived(const SensorSample &sample)
{
    processSample(sample);
}

// =========================================================================
// 2. processSample (Internal Helper)
// =========================================================================
// Triggered internally by: AppController::onSampleReceived()
// Role: Copies incoming sensor metrics (acceleration axes, temperature, anomaly score,
//       operating conditions) into the controller's instance variables. It aggregates
//       averages for the current timer window (to prevent GUI rendering choke) and
//       detects changes in machine state (NORMAL, WARNING, CRITICAL) to trigger alerts.
void AppController::processSample(const SensorSample &sample)
{
    const bool stateUpdated = sample.state != m_state || sample.operatingCondition != m_operatingCondition;
    m_latestSample = sample;
    m_state = sample.state;
    m_operatingCondition = sample.operatingCondition;
    m_anomalyScore = sample.anomalyScore;
    m_x = sample.x;
    m_y = sample.y;
    m_z = sample.z;
    m_temp = sample.temp;

    m_windowTempSum += m_temp;
    m_windowAnomalySum += m_anomalyScore;
    m_windowXSum += m_x;
    m_windowYSum += m_y;
    m_windowZSum += m_z;
    ++m_windowSampleCount;
    m_pendingLiveRefresh = true;

    if (stateUpdated) {
        emit stateChanged();
        appendLog(QStringLiteral("State: %1, Operating Condition: %2").arg(m_state, m_operatingCondition));
    }
}

// =========================================================================
// 3. flushLiveData (Internal Timer Callback Slot)
// =========================================================================
// Triggered by: m_liveDataTimer timeout signal (Line 25 of app_controller.cpp, fires every 50ms)
// Role: Averages the raw readings collected during the 50ms interval and appends the resulting
//       aggregated point to rolling live chart vectors:
//       - `anomalyValues`: Feeds "Score vs Time" LiveTrendChart in Dashboard.qml (Line 130)
//       - `xAxisValues`, `yAxisValues`, `zAxisValues`: Feeds the select-axis LiveTrendChart (Line 157)
//       It also invokes `storeHistorySample` to save the telemetry to the SQLite database.
void AppController::flushLiveData()
{
    if (!m_pendingLiveRefresh && m_windowSampleCount == 0)
        return;

    if (m_windowSampleCount > 0) {
        const double aggregatedTemp = m_windowTempSum / m_windowSampleCount;
        const double aggregatedAnomaly = m_windowAnomalySum / m_windowSampleCount;
        const double aggregatedX = m_windowXSum / m_windowSampleCount;
        const double aggregatedY = m_windowYSum / m_windowSampleCount;
        const double aggregatedZ = m_windowZSum / m_windowSampleCount;

        appendValue(m_anomalyValues, aggregatedAnomaly, MaxHistory);
        appendValue(m_xAxisValues, aggregatedX, MaxHistory);
        appendValue(m_yAxisValues, aggregatedY, MaxHistory);
        appendValue(m_zAxisValues, aggregatedZ, MaxHistory);

        emit anomalyValuesChanged();
        emit xAxisValuesChanged();
        emit yAxisValuesChanged();
        emit zAxisValuesChanged();

        storeHistorySample(aggregatedAnomaly, aggregatedX, aggregatedY, aggregatedZ, aggregatedTemp);
    }

    m_windowSampleCount = 0;
    m_windowTempSum = 0.0;
    m_windowAnomalySum = 0.0;
    m_windowXSum = 0.0;
    m_windowYSum = 0.0;
    m_windowZSum = 0.0;

    if (m_pendingLiveRefresh)
        emit dataChanged();

    m_pendingLiveRefresh = false;
}
