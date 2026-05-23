#include "app_controller.h"

// Live sample path: raw UART samples become current values, chart buffers, and score condition.
void AppController::onSampleReceived(const SensorSample &sample)
{
    processSample(sample);
}

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
    m_ambientTemp = sample.ambientTemp;

    m_windowTempSum += m_temp;
    m_windowAnomalySum += m_anomalyScore;
    m_windowXSum += m_x;
    m_windowYSum += m_y;
    m_windowZSum += m_z;
    ++m_windowSampleCount;
    m_pendingLiveRefresh = true;

    emit lastUpdateTextChanged();

    if (stateUpdated) {
        emit stateChanged();
        appendLog(QStringLiteral("State: %1, Operating Condition: %2").arg(m_state, m_operatingCondition));
    }
}

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
        appendValue(m_temperatureValues, aggregatedTemp, MaxHistory);
        appendValue(m_xAxisValues, aggregatedX, MaxHistory);
        appendValue(m_yAxisValues, aggregatedY, MaxHistory);
        appendValue(m_zAxisValues, aggregatedZ, MaxHistory);

        emit anomalyValuesChanged();
        emit temperatureValuesChanged();
        emit xAxisValuesChanged();
        emit yAxisValuesChanged();
        emit zAxisValuesChanged();

        updateCondition();
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

void AppController::updateCondition()
{
    const QString nextCondition = m_latestSample.state.isEmpty()
                                      ? SensorSample::stateForScore(m_anomalyScore)
                                      : m_latestSample.state;
    if (m_condition == nextCondition)
        return;

    m_condition = nextCondition;
    m_conditionTone = toneForCondition(m_condition);
    appendLog(QStringLiteral("Condition: %1").arg(m_condition));
}

void AppController::updateLastUpdateText()
{
    emit lastUpdateTextChanged();
}
