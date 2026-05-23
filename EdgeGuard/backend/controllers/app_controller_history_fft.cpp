#include "app_controller.h"

// FFT data is optional history metadata used by the lower spectrum chart on the History page.
namespace {
constexpr int HistoryFftWindowSize = 256;

// Converts numeric vectors into QVariantLists that QML can consume directly.
QVariantList toVariantList(const QVector<double> &values)
{
    QVariantList list;
    list.reserve(values.size());
    for (double value : values)
        list.append(value);
    return list;
}

// Estimates sample frequency from the first and last timestamps in a window.
double estimateSampleRateHz(const QVector<qint64> &timestampsMs)
{
    if (timestampsMs.size() < 2)
        return 0.0;

    const qint64 durationMs = timestampsMs.last() - timestampsMs.first();
    if (durationMs <= 0)
        return 0.0;

    return ((timestampsMs.size() - 1) * 1000.0) / durationMs;
}
}

// Computes the latest acceleration FFT window and appends spectrum metadata.
void AppController::appendHistoryFftData(QVariantMap &data,
                                         const QVector<double> &accelXValues,
                                         const QVector<double> &accelYValues,
                                         const QVector<double> &accelZValues,
                                         const QVector<qint64> &timestampsMs) const
{
    if (accelXValues.size() < HistoryFftWindowSize)
        return;

    const int startIndex = accelXValues.size() - HistoryFftWindowSize;
    const QVector<double> fftXWindow(accelXValues.begin() + startIndex, accelXValues.end());
    const QVector<double> fftYWindow(accelYValues.begin() + startIndex, accelYValues.end());
    const QVector<double> fftZWindow(accelZValues.begin() + startIndex, accelZValues.end());
    const QVector<qint64> fftTimeWindow(timestampsMs.begin() + startIndex, timestampsMs.end());
    const double sampleRateHz = estimateSampleRateHz(fftTimeWindow);
    if (sampleRateHz <= 0.0)
        return;

    const SignalProcessingService::FFTResult xResult = m_signalProcessingService.computeFFT(fftXWindow, sampleRateHz);
    const SignalProcessingService::FFTResult yResult = m_signalProcessingService.computeFFT(fftYWindow, sampleRateHz);
    const SignalProcessingService::FFTResult zResult = m_signalProcessingService.computeFFT(fftZWindow, sampleRateHz);

    data.insert(QStringLiteral("fftFrequencies"), toVariantList(xResult.frequencies));
    data.insert(QStringLiteral("fftXMagnitudes"), toVariantList(xResult.magnitudes));
    data.insert(QStringLiteral("fftYMagnitudes"), toVariantList(yResult.magnitudes));
    data.insert(QStringLiteral("fftZMagnitudes"), toVariantList(zResult.magnitudes));
    data.insert(QStringLiteral("fftXDominantFrequency"), xResult.dominantFrequency);
    data.insert(QStringLiteral("fftYDominantFrequency"), yResult.dominantFrequency);
    data.insert(QStringLiteral("fftZDominantFrequency"), zResult.dominantFrequency);
    data.insert(QStringLiteral("fftXEnergy"), xResult.energy);
    data.insert(QStringLiteral("fftYEnergy"), yResult.energy);
    data.insert(QStringLiteral("fftZEnergy"), zResult.energy);
    data.insert(QStringLiteral("fftMaxFrequency"), sampleRateHz / 2.0);
}
