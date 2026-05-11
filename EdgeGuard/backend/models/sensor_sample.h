#ifndef SENSOR_SAMPLE_H
#define SENSOR_SAMPLE_H

#include <QDateTime>
#include <QString>

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

    static QString stateForScore(double anomalyScore)
    {
        if (anomalyScore >= 80.0)
            return QStringLiteral("NORMAL");
        if (anomalyScore >= 40.0)
            return QStringLiteral("WARNING");
        return QStringLiteral("CRITICAL");
    }

    static QString stateForCode(int stateCode)
    {
        switch (stateCode) {
        case 0:
            return QStringLiteral("NORMAL");
        case 1:
            return QStringLiteral("WARNING");
        case 2:
            return QStringLiteral("CRITICAL");
        default:
            return QString();
        }
    }
};

Q_DECLARE_METATYPE(SensorSample)

#endif
