#ifndef DATA_STORAGE_SERVICE_H
#define DATA_STORAGE_SERVICE_H

#include "../models/sensor_sample.h"

#include <QString>

class DataStorageService
{
public:
    DataStorageService();

    void appendSample(const SensorSample &sample);
    void cleanOldData();
    QString loadLast24h() const;
    bool exportCsv(const QString &destinationPath) const;
    QString storagePath() const { return m_storagePath; }

private:
    QString m_storagePath;

    void ensureStorageFile() const;
    QString loadFilteredCsv() const;
};

#endif
