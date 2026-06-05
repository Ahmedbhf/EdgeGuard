#ifndef DATA_STORAGE_SERVICE_H
#define DATA_STORAGE_SERVICE_H

#include "../models/sensor_sample.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>

class DataStorageService
{
public:
    DataStorageService();
    ~DataStorageService();

    struct HistoryChunk {
        QVector<SensorSample> samples;
        int totalCount = 0;
        int offset = 0;
    };

    void appendSample(const SensorSample &sample);
    void cleanOldData();
    HistoryChunk loadLast24hSamples(int limit, int offset, const QString &deviceId = QString()) const;
    bool exportCsv(const QString &destinationPath, const QString &deviceId = QString()) const;
    // Returns the SQLite file path used for the rolling history database.
    QString storagePath() const { return m_storagePath; }

private:
    QString m_storagePath;
    QString m_connectionName;
    mutable QSqlDatabase m_database;
    mutable bool m_schemaReady = false;

    bool ensureDatabase() const;
    bool ensureSchema() const;
    HistoryChunk queryLast24hSamples(int limit, int offset, const QString &deviceId) const;
};

#endif
