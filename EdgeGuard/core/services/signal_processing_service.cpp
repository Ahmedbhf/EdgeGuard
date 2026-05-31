#include "signal_processing_service.h"

#include <kiss_fft.h>

#include <cmath>

SignalProcessingService::FFTResult SignalProcessingService::computeFFT(const QVector<double> &samples,
                                                                       double sampleRateHz) const
{
    FFTResult result;
    const int sampleCount = samples.size();
    if (sampleCount <= 1 || sampleRateHz <= 0.0)
        return result;

    kiss_fft_cfg fftConfig = kiss_fft_alloc(sampleCount, 0, nullptr, nullptr);
    if (!fftConfig)
        return result;

    QVector<kiss_fft_cpx> input(sampleCount);
    QVector<kiss_fft_cpx> output(sampleCount);

    double mean = 0.0;
    for (double sample : samples)
        mean += sample;
    mean /= sampleCount;

    for (int index = 0; index < sampleCount; ++index) {
        input[index].r = samples[index] - mean;
        input[index].i = 0.0;
    }

    kiss_fft(fftConfig, input.constData(), output.data());
    kiss_fft_free(fftConfig);

    const int uniqueBinCount = (sampleCount / 2) + 1;
    result.frequencies.reserve(uniqueBinCount);
    result.magnitudes.reserve(uniqueBinCount);

    double maxNonDcMagnitude = -1.0;
    int dominantNonDcIndex = 0;

    for (int index = 0; index < uniqueBinCount; ++index) {
        const double magnitude = std::sqrt((output[index].r * output[index].r)
                                           + (output[index].i * output[index].i));
        const double frequency = (index * sampleRateHz) / sampleCount;

        result.frequencies.append(frequency);
        result.magnitudes.append(magnitude);
        result.energy += magnitude;

        if (index > 0 && magnitude > maxNonDcMagnitude) {
            maxNonDcMagnitude = magnitude;
            dominantNonDcIndex = index;
        }
    }

    const int dominantIndex = maxNonDcMagnitude > 1e-9 ? dominantNonDcIndex : 0;
    result.dominantFrequency = (dominantIndex * sampleRateHz) / sampleCount;
    return result;
}
