#include "app_controller.h"

// Serial-port and setup identity logic: everything needed to enter/leave the device session.
void AppController::connectToPort(const QString &portName)
{
    const QString port = portName.split(' ').first().trimmed();
    if (port.isEmpty()) {
        appendLog(QStringLiteral("No serial device selected."));
        return;
    }

    setSelectedPort(port);
    if (m_serialService.connectToPort(port))
        appendLog(QStringLiteral("Connected to %1").arg(port));
}

void AppController::connectPreferredPort(const QString &preferredPort)
{
    refreshPorts();

    QString targetPort;
    for (int index = 0; index < availablePorts().size(); ++index) {
        if (portNameAt(index) == preferredPort.trimmed()) {
            targetPort = preferredPort.trimmed();
            break;
        }
    }

    if (targetPort.isEmpty() && !m_selectedPort.isEmpty())
        targetPort = m_selectedPort;
    if (targetPort.isEmpty() && !availablePorts().isEmpty())
        targetPort = portNameAt(0);
    if (targetPort.isEmpty())
        targetPort = preferredPort.trimmed();

    connectToPort(targetPort);
}

void AppController::disconnectPort()
{
    if (connected())
        m_serialService.disconnectPort();
}

void AppController::disconnectAndReset()
{
    disconnectPort();
    resetDeviceIdentity();
}

void AppController::setSelectedPort(const QString &portName)
{
    const QString port = portName.trimmed();
    if (m_selectedPort == port)
        return;

    m_selectedPort = port;
    emit selectedPortChanged();
}

void AppController::resetDeviceIdentity()
{
    bool deviceChanged = false;
    if (!m_deviceId.isEmpty()) {
        m_deviceId.clear();
        deviceChanged = true;
    }

    if (!m_machineType.isEmpty()) {
        m_machineType.clear();
        emit machineTypeChanged();
    }

    if (deviceChanged)
        emit deviceIdChanged();
}

void AppController::setMachineType(const QString &machineType)
{
    const QString trimmedMachineType = machineType.trimmed();
    if (m_machineType == trimmedMachineType)
        return;

    m_machineType = trimmedMachineType;
    emit machineTypeChanged();
}

void AppController::syncPorts()
{
    emit availablePortsChanged();

    QString nextPort = m_selectedPort;
    bool keepSelectedPort = false;
    for (int index = 0; index < availablePorts().size(); ++index)
        keepSelectedPort |= portNameAt(index) == m_selectedPort;

    if (!keepSelectedPort)
        nextPort = availablePorts().isEmpty() ? QString() : portNameAt(0);

    if (nextPort != m_selectedPort) {
        m_selectedPort = nextPort;
        emit selectedPortChanged();
    }
}

void AppController::syncConnection()
{
    if (!connected()) {
        flushLiveData();
        appendLog(QStringLiteral("Disconnected from %1").arg(m_selectedPort.isEmpty() ? QStringLiteral("device") : m_selectedPort));
    }

    emit connectedChanged();
    emit relearnAvailabilityChanged();
}

void AppController::requestRelearn()
{
    if (!connected()) {
        appendLog(QStringLiteral("Connect a serial device before relearning."));
        return;
    }

    if (m_relearnCooldownSeconds > 0) {
        appendLog(QStringLiteral("Relearn is cooling down for %1 s.").arg(m_relearnCooldownSeconds));
        return;
    }

    if (!m_serialService.writeData(QByteArrayLiteral("r")))
        return;

    m_relearnCooldownSeconds = RelearnCooldownSeconds;
    m_relearnCooldownTimer.start();
    appendLog(QStringLiteral("Sent relearn command."));
    emit relearnCooldownChanged();
    emit relearnAvailabilityChanged();
}

void AppController::tickRelearnCooldown()
{
    if (m_relearnCooldownSeconds <= 0) {
        m_relearnCooldownTimer.stop();
        return;
    }

    --m_relearnCooldownSeconds;
    if (m_relearnCooldownSeconds <= 0)
        m_relearnCooldownTimer.stop();

    emit relearnCooldownChanged();
    emit relearnAvailabilityChanged();
}
