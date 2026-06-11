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
    QString operatingCondition;
    bool isLearning = false;
    int nlValue = 0;

    // Converts an anomaly score into the state labels expected by the UI.
    static QString stateForScore(double anomalyScore)
    {
        if (anomalyScore >= 80.0)
            return QStringLiteral("NORMAL");
        if (anomalyScore >= 40.0)
            return QStringLiteral("WARNING");
        return QStringLiteral("CRITICAL");
    }

    // Converts the numeric state code received over UART into a state label.
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

    // Converts the numeric fault class into a user-facing operating condition.
    static QString operatingConditionForCode(int classCode, const QString &state)
    {
        if (state == QStringLiteral("NORMAL"))
            return classCode == 3 ? QStringLiteral("Motor Off") : QStringLiteral("Motor On");

        switch (classCode) {
        case 0:
            return QStringLiteral("Imbalance");
        case 1:
            return QStringLiteral("Friction");
        case 2:
            return QStringLiteral("Loose Belt");
        case 3:
            return QStringLiteral("Motor Off");
        default:
            return QString();
        }
    }
};

Q_DECLARE_METATYPE(SensorSample)

#endif
