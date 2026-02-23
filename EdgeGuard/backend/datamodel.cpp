#include "datamodel.h"
#include <cmath>
#include <QTimer>
#include <QRandomGenerator>

DataModel::DataModel(QObject *parent)
    : QObject(parent),
    m_x(0),
    m_y(0),
    m_z(0),
    m_rms(0),
    m_peak2peak(0),
    m_variance(0),
    m_crestFactor(0),
    m_temp(40),
    m_tempSlope(0),
    m_lastTemp(40),
    m_zScore(0),
    m_meanRms(1.10),
    m_stdRms(0.15)
{
    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, [this]() {

        double x = QRandomGenerator::global()->bounded(4.0) - 2.0;
        double y = QRandomGenerator::global()->bounded(4.0) - 2.0;
        double z = QRandomGenerator::global()->bounded(4.0) - 2.0;

        updateRaw(x, y, z);

    });

    timer->start(1000);
}

double DataModel::rms() const
{
    return m_rms;
}

double DataModel::peak2peak() const
{
    return m_peak2peak;
}

double DataModel::variance() const
{
    return m_variance;
}

double DataModel::crestFactor() const
{
    return m_crestFactor;
}

double DataModel::temp() const
{
    return m_temp;
}

double DataModel::tempSlope() const
{
    return m_tempSlope;
}
double DataModel::zScore() const
{
    return m_zScore;
}
void DataModel::updateRaw(double x, double y, double z)
{
    m_x = x;
    m_y = y;
    m_z = z;

    compute();

    emit dataChanged();
}

void DataModel::compute()
{
    m_rms = std::sqrt((m_x*m_x + m_y*m_y + m_z*m_z) / 3.0);

    if (m_stdRms != 0)
        m_zScore = (m_rms - m_meanRms) / m_stdRms;
    else
        m_zScore = 0;

    m_peak2peak = std::max({m_x, m_y, m_z}) - std::min({m_x, m_y, m_z});

    m_variance = (m_x*m_x + m_y*m_y + m_z*m_z) / 3.0 - (m_rms * m_rms);

    if (m_rms != 0)
        m_crestFactor = m_peak2peak / m_rms;
    else
        m_crestFactor = 0;
}
