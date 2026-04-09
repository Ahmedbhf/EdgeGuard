#include "signal_processing_service.h"

#include <cmath>
#include <utility>

namespace {
constexpr double Pi = 3.14159265358979323846;
}

SignalProcessingService::FFTResult SignalProcessingService::computeFFT(const QVector<double> &samples,
                                                                      double sampleRateHz) const
{
    FFTResult result;
    const int sampleCount = samples.size();
    if (sampleCount <= 1 || sampleRateHz <= 0.0 || !isPowerOfTwo(sampleCount))
        return result;

    QVector<ComplexValue> values(sampleCount);
    double mean = 0.0;
    for (double sample : samples)
        mean += sample;
    mean /= sampleCount;

    for (int index = 0; index < sampleCount; ++index)
        values[index].real = samples[index] - mean;

    int bitCount = 0;
    for (int size = sampleCount; size > 1; size >>= 1)
        ++bitCount;

    for (int index = 0; index < sampleCount; ++index) {
        const int reversedIndex = reverseBits(index, bitCount);
        if (reversedIndex > index)
            std::swap(values[index], values[reversedIndex]);
    }

    for (int length = 2; length <= sampleCount; length <<= 1) {
        const int halfLength = length / 2;
        const double angleStep = (-2.0 * Pi) / length;

        for (int offset = 0; offset < sampleCount; offset += length) {
            for (int index = 0; index < halfLength; ++index) {
                const double angle = angleStep * index;
                const double twiddleReal = std::cos(angle);
                const double twiddleImag = std::sin(angle);

                const ComplexValue even = values[offset + index];
                const ComplexValue odd = values[offset + index + halfLength];
                const double oddReal = (twiddleReal * odd.real) - (twiddleImag * odd.imag);
                const double oddImag = (twiddleReal * odd.imag) + (twiddleImag * odd.real);

                values[offset + index].real = even.real + oddReal;
                values[offset + index].imag = even.imag + oddImag;
                values[offset + index + halfLength].real = even.real - oddReal;
                values[offset + index + halfLength].imag = even.imag - oddImag;
            }
        }
    }

    const int uniqueBinCount = (sampleCount / 2) + 1;
    result.frequencies.reserve(uniqueBinCount);
    result.magnitudes.reserve(uniqueBinCount);

    double maxMagnitude = -1.0;
    int dominantIndex = 0;

    for (int index = 0; index < uniqueBinCount; ++index) {
        const double magnitude = std::sqrt((values[index].real * values[index].real)
                                           + (values[index].imag * values[index].imag));
        const double frequency = (index * sampleRateHz) / sampleCount;

        result.frequencies.append(frequency);
        result.magnitudes.append(magnitude);
        result.energy += magnitude;

        if (magnitude > maxMagnitude) {
            maxMagnitude = magnitude;
            dominantIndex = index;
        }
    }

    result.dominantFrequency = (dominantIndex * sampleRateHz) / sampleCount;
    return result;
}

bool SignalProcessingService::isPowerOfTwo(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

int SignalProcessingService::reverseBits(int value, int bitCount)
{
    int reversed = 0;
    for (int index = 0; index < bitCount; ++index) {
        reversed = (reversed << 1) | (value & 1);
        value >>= 1;
    }
    return reversed;
}
