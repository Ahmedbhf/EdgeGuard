#ifndef SIGNAL_PROCESSING_SERVICE_H
#define SIGNAL_PROCESSING_SERVICE_H

#include <QVector>

class SignalProcessingService
{
public:
    struct FFTResult {
        QVector<double> frequencies;
        QVector<double> magnitudes;
        double dominantFrequency = 0.0;
        double energy = 0.0;
    };

    FFTResult computeFFT(const QVector<double> &samples, double sampleRateHz) const;

private:
    struct ComplexValue {
        double real = 0.0;
        double imag = 0.0;
    };

    static bool isPowerOfTwo(int value);
    static int reverseBits(int value, int bitCount);
};

#endif
