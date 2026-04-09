#include "data_storage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

namespace {
constexpr qint64 RollingWindowMs = 24LL * 60LL * 60LL * 1000LL;
const char *HeaderLine = "timestamp,anomaly,x,y,z,temp\n";

QString resolveStoragePath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QDir::currentPath();

    QDir dir(basePath);
    dir.mkpath(".");
    return dir.filePath(QStringLiteral("rolling_24h_history.csv"));
}

QDateTime parseTimestampUtc(const QString &field)
{
    const QString trimmed = field.trimmed();
    QDateTime timestamp = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!timestamp.isValid())
        timestamp = QDateTime::fromString(trimmed, Qt::ISODate);
    if (timestamp.isValid())
        timestamp = timestamp.toUTC();
    return timestamp;
}
}

DataStorage::DataStorage() : m_storagePath(resolveStoragePath())
{
    ensureStorageFile();
    cleanOldData();
}

void DataStorage::appendData(const DataPoint &point)
{
    ensureStorageFile();

    QFile file(m_storagePath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << point.timestampUtc.toString(Qt::ISODateWithMs)
           << ','
           << QString::number(point.anomaly, 'f', 3)
           << ','
           << QString::number(point.x, 'f', 4)
           << ','
           << QString::number(point.y, 'f', 4)
           << ','
           << QString::number(point.z, 'f', 4)
           << ','
           << QString::number(point.temp, 'f', 2)
           << '\n';
}

void DataStorage::cleanOldData()
{
    const QString filteredCsv = loadFilteredCsv();
    if (filteredCsv.isEmpty())
        return;

    QSaveFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    file.write(filteredCsv.toUtf8());
    file.commit();
}

QString DataStorage::loadLast24h() const
{
    return loadFilteredCsv();
}

bool DataStorage::exportCsv(const QString &destinationPath) const
{
    const QString targetPath = destinationPath.trimmed();
    if (targetPath.isEmpty())
        return false;

    const QString filteredCsv = loadFilteredCsv();
    if (filteredCsv.isEmpty())
        return false;

    QFileInfo targetInfo(targetPath);
    QDir().mkpath(targetInfo.absolutePath());

    QSaveFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(filteredCsv.toUtf8());
    return file.commit();
}

void DataStorage::ensureStorageFile() const
{
    QFileInfo info(m_storagePath);
    QDir().mkpath(info.absolutePath());

    QFile file(m_storagePath);
    if (file.exists() && file.size() > 0)
        return;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    file.write(HeaderLine);
}

QString DataStorage::loadFilteredCsv() const
{
    ensureStorageFile();

    QFile file(m_storagePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString::fromLatin1(HeaderLine);

    QTextStream stream(&file);
    const qint64 cutoffMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - RollingWindowMs;
    QStringList rows;
    rows.append(QString::fromLatin1(HeaderLine).trimmed());

    bool firstLine = true;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        if (firstLine) {
            firstLine = false;
            if (line.startsWith(QStringLiteral("timestamp,")))
                continue;
        }

        const QStringList fields = line.split(',');
        if (fields.size() < 6)
            continue;

        const QDateTime timestampUtc = parseTimestampUtc(fields[0]);
        if (!timestampUtc.isValid())
            continue;

        if (timestampUtc.toMSecsSinceEpoch() < cutoffMs)
            continue;

        rows.append(line);
    }

    return rows.join('\n') + '\n';
}
