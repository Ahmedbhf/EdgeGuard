#include "app_controller.h"

// Serial-port and setup identity logic: everything needed to enter/leave the device session.

// =========================================================================
// 1. connectToPort (Internal Helper)
// =========================================================================
// Triggered internally by: AppController::connectPreferredPort()
// Role: Extracts the clean COM/UART port name (removing details) and instructs the
//       underlying SerialService to open the port. It logs the result of the connection.
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

// =========================================================================
// 2. connectPreferredPort (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Connect Device" ControlButton in SetupPage.qml (Line 108)
//               via the Javascript helper root.beginDeviceDetection() (Line 39)
// Role: Refreshes system serial ports and loops through available ports to find
//       a match for the preferredPort ("COM11"). If the preferred port is not found,
//       it falls back to the last selected port or the first detected port. It then
//       calls connectToPort() to open the UART communication stream.
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

// =========================================================================
// 3. disconnectPort (Internal Helper)
// =========================================================================
// Triggered internally by: AppController::disconnectAndReset()
// Role: Instructs the SerialService to close the currently open COM/UART port.
void AppController::disconnectPort()
{
    if (connected())
        m_serialService.disconnectPort();
}

// =========================================================================
// 4. disconnectAndReset (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Disconnect" ControlButton in DashboardHeaderBar.qml (Line 59 & 122)
//               via the onConnectionToggled signal handler in Dashboard.qml (Line 50)
// Role: Disconnects from the serial device via disconnectPort() and resets the remembered
//       device session properties (clearing device ID and machine type) via resetDeviceIdentity().
//       This permits the user to return to the Setup Page to connect a new device.
void AppController::disconnectAndReset()
{
    disconnectPort();
    resetDeviceIdentity();
}

// =========================================================================
// 5. setSelectedPort (Internal Helper)
// =========================================================================
// Triggered internally by: AppController::connectToPort()
// Role: Remembers the selected COM port name for reconnects and avoids redundant state updates.
void AppController::setSelectedPort(const QString &portName)
{
    const QString port = portName.trimmed();
    if (m_selectedPort == port)
        return;

    m_selectedPort = port;
}

// =========================================================================
// 6. resetDeviceIdentity (Internal Helper)
// =========================================================================
// Triggered internally by: AppController::disconnectAndReset()
// Role: Clears the cached device id string and machine type string. Emits the QML notifier
//       signals `deviceIdChanged()` and `machineTypeChanged()` to reset setup assistant status labels.
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

// =========================================================================
// 7. setMachineType (Internal Property Setter)
// =========================================================================
// Triggered by: SetupPage.qml selected machine type grid card binding (Line 127) and
//               internally when a device ID packet is initially received over serial (defaults to "DC Motor").
// Role: Updates the current session's machine type classification (e.g. DC Motor, Pump, Fan)
//       and emits `machineTypeChanged()` to update dashboard labels and icons.
void AppController::setMachineType(const QString &machineType)
{
    const QString trimmedMachineType = machineType.trimmed();
    if (m_machineType == trimmedMachineType)
        return;

    m_machineType = trimmedMachineType;
    emit machineTypeChanged();
}

// =========================================================================
// 8. syncPorts (Internal Slot)
// =========================================================================
// Triggered by: SerialService::portsChanged signal (when a USB-to-UART bridge is plugged/unplugged)
// Role: Emits `availablePortsChanged()` to update the ports list in the UI, and updates the active
//       selected port to a fallback if the previous port is no longer available on the system.
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
    }
}

// =========================================================================
// 9. syncConnection (Internal Slot)
// =========================================================================
// Triggered by: SerialService::connectedChanged signal
// Role: Dispatches notifications to the QML frontend (connectedChanged, relearnAvailabilityChanged).
//       If the connection was severed, it flushes live data, stops live updates, and logs the disconnection.
void AppController::syncConnection()
{
    if (!connected()) {
        flushLiveData();
        appendLog(QStringLiteral("Disconnected from %1").arg(m_selectedPort.isEmpty() ? QStringLiteral("device") : m_selectedPort));
    }

    emit connectedChanged();
    emit relearnAvailabilityChanged();
}

// =========================================================================
// 10. requestRelearn (Q_INVOKABLE)
// =========================================================================
// Triggered by: "Relearn" ControlButton in DashboardHeaderBar.qml (Line 64 & 128)
//               via the onRelearnClicked signal handler in Dashboard.qml (Line 56)
// Role: Writes the ASCII character 'r' over the open serial port to trigger the STM32's
//       on-device model retraining routine. It also triggers a 40-second cooling lockout
//       period (updating cooldown timer values in the UI) to avoid serial transmission spam.
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

// =========================================================================
// 11. tickRelearnCooldown (Internal Slot)
// =========================================================================
// Triggered by: m_relearnCooldownTimer timeout signal (once per second)
// Role: Decrements m_relearnCooldownSeconds and fires notification signals to decrement the
//       lockout duration shown on the "Relearn" button in the UI. Automatically stops the
//       timer and re-enables the button when the count reaches zero.
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
