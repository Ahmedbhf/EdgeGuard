#include "app_controller.h"

#include <algorithm>
#include <cmath>

namespace {
struct HistorySeries {
    QVariantList scorePoints;
    QVariantList tempPoints;
    QVariantList accelXPoints;
    QVariantList accelYPoints;
    QVariantList accelZPoints;
    QVector<double> tempValues;
    QVector<double> accelXValues;
    QVector<double> accelYValues;
    QVector<double> accelZValues;
    QVector<qint64> accelTimestampsMs;
    qint64 firstMs = -1;
    qint64 lastMs = -1;
};

QVariantMap chartPoint(qint64 x, double y)
{
    return {
        { QStringLiteral("x"), x },
        { QStringLiteral("y"), y }
    };
}

QVariantMap computePaddedRange(const QVector<double> &values, bool clampMinToZero)
{
    if (values.isEmpty())
        return { { QStringLiteral("min"), 0.0 }, { QStringLiteral("max"), 1.0 } };

    double minValue = values.first();
    double maxValue = values.first();
    for (double value : values) {
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    if (qFuzzyCompare(minValue, maxValue)) {
        const double padding = std::max(1.0, std::abs(minValue) * 0.2);
        const double nextMin = minValue - padding;
        return {
            { QStringLiteral("min"), clampMinToZero ? std::max(0.0, nextMin) : nextMin },
            { QStringLiteral("max"), maxValue + padding }
        };
    }

    const double spread = maxValue - minValue;
    const double nextMin = minValue - spread * 0.12;
    return {
        { QStringLiteral("min"), clampMinToZero ? std::max(0.0, nextMin) : nextMin },
        { QStringLiteral("max"), maxValue + spread * 0.12 }
    };
}

void appendRange(QVariantMap &data, const QString &prefix, const QVariantMap &range)
{
    data.insert(prefix + QStringLiteral("MinY"), range.value(QStringLiteral("min")));
    data.insert(prefix + QStringLiteral("MaxY"), range.value(QStringLiteral("max")));
}

void appendSample(HistorySeries &series, const SensorSample &sample)
{
    const qint64 pointMs = sample.timestampUtc.toUTC().toMSecsSinceEpoch();
    if (series.firstMs < 0)
        series.firstMs = pointMs;
    series.lastMs = pointMs;

    series.scorePoints.append(chartPoint(pointMs, sample.anomalyScore));
    series.tempPoints.append(chartPoint(pointMs, sample.temp));
    series.accelXPoints.append(chartPoint(pointMs, sample.x));
    series.accelYPoints.append(chartPoint(pointMs, sample.y));
    series.accelZPoints.append(chartPoint(pointMs, sample.z));

    series.tempValues.append(sample.temp);
    series.accelXValues.append(sample.x);
    series.accelYValues.append(sample.y);
    series.accelZValues.append(sample.z);
    series.accelTimestampsMs.append(pointMs);
}

void appendChartData(QVariantMap &data, const HistorySeries &series)
{
    const qint64 safeEndMs = series.lastMs > series.firstMs ? series.lastMs : series.firstMs + 1000;

    data.insert(QStringLiteral("anomalyPoints"), series.scorePoints);
    data.insert(QStringLiteral("tempPoints"), series.tempPoints);
    data.insert(QStringLiteral("accelXPoints"), series.accelXPoints);
    data.insert(QStringLiteral("accelYPoints"), series.accelYPoints);
    data.insert(QStringLiteral("accelZPoints"), series.accelZPoints);
    data.insert(QStringLiteral("anomalyMinY"), 0.0);
    data.insert(QStringLiteral("anomalyMaxY"), 100.0);
    appendRange(data, QStringLiteral("temp"), computePaddedRange(series.tempValues, true));
    appendRange(data, QStringLiteral("accelX"), computePaddedRange(series.accelXValues, false));
    appendRange(data, QStringLiteral("accelY"), computePaddedRange(series.accelYValues, false));
    appendRange(data, QStringLiteral("accelZ"), computePaddedRange(series.accelZValues, false));
    data.insert(QStringLiteral("fullStartMs"), series.firstMs);
    data.insert(QStringLiteral("fullEndMs"), safeEndMs);
    data.insert(QStringLiteral("minimumWindowMs"),
                std::max<qint64>(1000, (safeEndMs - series.firstMs) / std::min<qint64>(20, series.scorePoints.size())));
    data.insert(QStringLiteral("sampleCount"), series.scorePoints.size());
}
}

AppController::ParsedHistory AppController::parseHistorySamples(const QVector<SensorSample> &samples) const
{
    ParsedHistory parsedHistory;
    if (samples.isEmpty()) {
        parsedHistory.statusText = QStringLiteral("No stored history is available yet.");
        return parsedHistory;
    }

    // HistorySeries keeps two forms of each signal:
    // QVariantList points go directly to QML charts; QVector values are for ranges and FFT math.
    HistorySeries series;
    for (const SensorSample &sample : samples) {
        if (sample.timestampUtc.isValid())
            appendSample(series, sample);
    }

    if (series.scorePoints.isEmpty()) {
        parsedHistory.statusText = QStringLiteral("No samples stored in the last 24 hours yet.");
        return parsedHistory;
    }

    appendChartData(parsedHistory.data, series);
    appendHistoryFftData(parsedHistory.data,
                         series.accelXValues,
                         series.accelYValues,
                         series.accelZValues,
                         series.accelTimestampsMs);

    parsedHistory.statusText = QStringLiteral("%1 samples loaded from the local 24-hour history database.")
                                   .arg(series.scorePoints.size());
    return parsedHistory;
}
