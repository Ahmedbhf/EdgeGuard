#ifndef SENSOR_SAMPLE_H
#define SENSOR_SAMPLE_H

#include <QDateTime>
#include <QString>

#include <cmath>

struct SensorSample
{
    QString deviceId;
    QDateTime timestampUtc;
    double anomalyScore = 0.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double temp = 0.0;
    double ambientTemp = 0.0;
    QString state;

    double rms() const
    {
        const double energy = ((x * x) + (y * y) + (z * z)) / 3.0;
        return std::sqrt(energy);
    }

    static QString stateForScore(double anomalyScore)
    {
        if (anomalyScore >= 80.0)
            return QStringLiteral("NORMAL");
        if (anomalyScore >= 40.0)
            return QStringLiteral("WARNING");
        return QStringLiteral("FAULT");
    }
};

Q_DECLARE_METATYPE(SensorSample)

#endif
