#include "app_controller.h"
#include <QDebug>
#include <QTime>

// Small shared helpers kept out of the main controller file.
// Prints a timestamped message to the system debug console.
void AppController::appendLog(const QString &text)
{
    qDebug() << QStringLiteral("%1  %2").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), text);
}

// Appends a chart value while keeping the rolling buffer capped.
void AppController::appendValue(QVector<double> &values, double value, int maxHistory)
{
    values.append(value);
    if (values.size() > maxHistory)
        values.removeFirst();
}
