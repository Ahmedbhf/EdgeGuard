#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <QDateTime>
#include <QString>

class DataStorage
{
public:
    struct DataPoint {
        QDateTime timestampUtc;
        double anomaly = 0.0;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double temp = 0.0;
    };

    DataStorage();

    void appendData(const DataPoint &point);
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
