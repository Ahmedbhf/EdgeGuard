#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QObject>

class DataModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double rms READ rms NOTIFY dataChanged)
    Q_PROPERTY(double peak2peak READ peak2peak NOTIFY dataChanged)
    Q_PROPERTY(double variance READ variance NOTIFY dataChanged)
    Q_PROPERTY(double crestFactor READ crestFactor NOTIFY dataChanged)
    Q_PROPERTY(double temp READ temp NOTIFY dataChanged)
    Q_PROPERTY(double tempSlope READ tempSlope NOTIFY dataChanged)
    Q_PROPERTY(double zScore READ zScore NOTIFY dataChanged)

public:
    explicit DataModel(QObject *parent = nullptr);

    double rms() const;
    double peak2peak() const;
    double variance() const;
    double crestFactor() const;
    double temp() const;
    double tempSlope() const;
    double zScore() const;


    Q_INVOKABLE void updateRaw(double x, double y, double z);

signals:
    void dataChanged();

private:
    void compute();

    double m_x;
    double m_y;
    double m_z;

    double m_rms;
    double m_peak2peak;
    double m_variance;
    double m_crestFactor;
    double m_temp;
    double m_tempSlope;

    double m_lastTemp;
    double m_meanRms;
    double m_stdRms;
    double m_zScore;
};

#endif
