pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "components"
import EdgeGuard

Page {
    id: root

    property bool connectionRequested: false
    readonly property string preferredPort: "COM11"
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

    Component.onCompleted: Qt.callLater(root.continueToDashboard)

    function continueToDashboard() {
        if (!appController.deviceConnected)
            return

        StackView.view.push("Dashboard.qml")
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

            ControlButton {
                text: "Connect Device"
                width: 200
                height: 44
                primary: true
                onClicked: {
                    root.connectionRequested = true
                    appController.connectPreferredPort(root.preferredPort)
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
                        readonly property bool isSelected: appController.deviceConnected && appController.machineType === modelData.value

                        width: (machineGrid.width - (machineGrid.columns - 1) * machineGrid.spacing) / machineGrid.columns
                        height: 182
                        radius: 18
                        color: Theme.panel
                        opacity: !appController.deviceConnected ? 0.3 : (isSelected ? 1.0 : 0.2)
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
                text: "Device ID: " + (appController.deviceId.length > 0 ? appController.deviceId : "Waiting for UART")
                color: Theme.text
                font.pixelSize: 14
            }

            Text {
                text: "Machine: " + (appController.machineType.length > 0 ? appController.machineType : "Waiting for detection")
                color: Theme.text
                font.pixelSize: 14
            }

            Text {
                width: parent.width
                visible: root.connectionRequested && !appController.deviceConnected
                text: "Waiting for UART data..."
                color: Theme.muted
                font.pixelSize: 13
            }

            ControlButton {
                text: "Continue"
                width: 200
                height: 44
                enabled: appController.deviceConnected
                primary: true
                onClicked: root.continueToDashboard()
            }
        }
    }
}
