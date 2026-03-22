pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import EdgeGuard

Page {
    id: root

    // These values drive the simple setup flow before entering the main dashboard.
    property bool connectionRequested: false
    readonly property string preferredPort: "COM11"
    readonly property bool deviceConnected: dataModel.deviceId !== ""
    readonly property string assetsBasePath: "qrc:/qt/qml/EdgeGuard/assets/"

    property var machineCards: [
        { label: "DC Motor", value: "DC Motor", image: root.assetsBasePath + "motor.jpg" },
        { label: "Pump", value: "Pump", image: root.assetsBasePath + "pump.jpeg" },
        { label: "Fan", value: "Fan", image: root.assetsBasePath + "grid.jpeg" },
        { label: "Gearbox", value: "Gearbox", image: root.assetsBasePath + "gearbox.jpeg" },
        { label: "Compressor", value: "Compressor", image: root.assetsBasePath + "compresor.jpeg" },
        { label: "Other", value: "Other", image: root.assetsBasePath + "other.jpeg" }
    ]

    background: Rectangle {
        color: Theme.bg
    }

    function continueToDashboard() {
        // Only continue after the device has identified itself through UART.
        if (!root.deviceConnected)
            return

        StackView.view.push("Dashboard.qml")
    }

    function connectToDetectedPort() {
        // Try the preferred port first, then the current selection, then the first available port.
        dataModel.refreshPorts()

        var targetPort = ""

        for (var i = 0; i < dataModel.availablePorts.length; ++i) {
            if (dataModel.portNameAt(i) === root.preferredPort) {
                targetPort = root.preferredPort
                break
            }
        }

        if ((!targetPort || targetPort.length === 0) && dataModel.selectedPort.length > 0)
            targetPort = dataModel.selectedPort

        if ((!targetPort || targetPort.length === 0) && dataModel.availablePorts.length > 0)
            targetPort = dataModel.portNameAt(0)

        if (!targetPort || targetPort.length === 0)
            targetPort = root.preferredPort

        dataModel.setSelectedPort(targetPort)
        dataModel.connectToPort(targetPort)
    }

    Item {
        anchors.fill: parent

        Column {
            anchors.centerIn: parent
            width: 800
            spacing: 24

            Text {
                text: "Setup Assistant"
                color: Theme.text
                font.pixelSize: 28
                font.weight: Font.DemiBold
            }

            Text {
                width: parent.width
                text: "Connect the device to load its detected machine profile and continue to the dashboard."
                color: Theme.muted
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            Button {
                id: connectButton
                text: "Connect Device"
                width: 200
                height: 44
                onClicked: {
                    // Remember that the user started the connection flow so we can show waiting text.
                    root.connectionRequested = true
                    root.connectToDetectedPort()
                }

                background: Rectangle {
                    radius: 14
                    color: Theme.primary
                }

                contentItem: Text {
                    text: connectButton.text
                    color: Theme.primaryFg
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                    font.bold: true
                }
            }

            Grid {
                id: machineGrid
                columns: 3
                spacing: 16
                width: parent.width

                Repeater {
                    model: root.machineCards

                    delegate: Rectangle {
                        required property var modelData
                        // Cards are read-only here; they highlight the machine type detected from the device.
                        readonly property bool isSelected: root.deviceConnected && dataModel.machineType === modelData.value

                        width: (machineGrid.width - (machineGrid.columns - 1) * machineGrid.spacing) / machineGrid.columns
                        height: 182
                        radius: 18
                        color: Theme.panel
                        opacity: !root.deviceConnected ? 0.3 : (isSelected ? 1.0 : 0.2)
                        border.width: isSelected ? 2 : 0
                        border.color: Theme.primary

                        Column {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Rectangle {
                                width: parent.width
                                height: 118
                                radius: 14
                                color: Theme.panel2
                                clip: true

                                Image {
                                    id: cardImage
                                    anchors.fill: parent
                                    source: modelData.image
                                    fillMode: Image.PreserveAspectCrop
                                    cache: false
                                }

                                Text {
                                    anchors.centerIn: parent
                                    width: parent.width - 20
                                    text: modelData.label
                                    visible: cardImage.status !== Image.Ready
                                    color: Theme.text
                                    horizontalAlignment: Text.AlignHCenter
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                            }

                            Text {
                                width: parent.width
                                text: modelData.label
                                color: Theme.text
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
            }

            Text {
                text: "Device ID: " + (dataModel.deviceId.length > 0 ? dataModel.deviceId : "Waiting for UART")
                color: Theme.text
                font.pixelSize: 14
            }

            Text {
                text: "Machine: " + (dataModel.machineType.length > 0 ? dataModel.machineType : "Waiting for detection")
                color: Theme.text
                font.pixelSize: 14
            }

            Text {
                width: parent.width
                // This helper text only appears after the user asks to connect.
                visible: root.connectionRequested && !root.deviceConnected
                text: "Waiting for UART data..."
                color: Theme.muted
                font.pixelSize: 13
            }

            Button {
                id: continueButton
                text: "Continue"
                width: 200
                height: 44
                enabled: root.deviceConnected
                onClicked: root.continueToDashboard()

                background: Rectangle {
                    radius: 14
                    color: continueButton.enabled ? Theme.primary : Theme.borderSoft
                }

                contentItem: Text {
                    text: continueButton.text
                    color: Theme.primaryFg
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                    font.bold: true
                }
            }
        }
    }
}
