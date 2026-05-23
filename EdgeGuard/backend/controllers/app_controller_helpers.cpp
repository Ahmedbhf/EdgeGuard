#include "app_controller.h"

#include <QTime>

// Small shared helpers kept out of the main controller file.
// Adds a timestamped message to the bounded UI log buffer.
void AppController::appendLog(const QString &text)
{
    m_logs.append(QStringLiteral("%1  %2").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), text));
    while (m_logs.size() > MaxLogLines)
        m_logs.removeFirst();
    emit logTextChanged();
}

// Appends a chart value while keeping the rolling buffer capped.
void AppController::appendValue(QVector<double> &values, double value, int maxHistory)
{
    values.append(value);
    if (values.size() > maxHistory)
        values.removeFirst();
}

// Maps machine condition labels to visual tone tokens used in QML.
QString AppController::toneForCondition(const QString &condition)
{
    if (condition == QStringLiteral("NORMAL"))
        return QStringLiteral("ok");
    if (condition == QStringLiteral("WARNING"))
        return QStringLiteral("warning");
    return QStringLiteral("fault");
}
