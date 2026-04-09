import QtQuick
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    property string title: "Edge Maintenance Monitor"
    property bool compact: width < 1280
    readonly property bool stacked: width < 980

    signal connectionToggled()
    signal historyClicked()
    signal themeToggleClicked()

    implicitHeight: stacked ? 126 : 72
    height: implicitHeight
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    RowLayout {
        visible: !root.stacked
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text {
            text: root.title
            color: Theme.text
            font.pixelSize: root.compact ? 16 : 18
            font.bold: true
            elide: Text.ElideRight
            Layout.maximumWidth: root.compact ? 240 : 340
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            visible: appController.machineType.length > 0 || appController.deviceId.length > 0
            text: appController.machineType.length > 0
                  ? appController.machineType + " (" + (appController.deviceId.length > 0 ? appController.deviceId : "Waiting for UID") + ")"
                  : "Waiting for UID"
            color: Theme.muted
            font.pixelSize: 13
            elide: Text.ElideRight
            Layout.maximumWidth: root.compact ? 280 : 420
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            ControlButton {
                text: appController.connected ? "Disconnect" : "Connect"
                primary: appController.connected
                onClicked: root.connectionToggled()
            }

            ControlButton {
                text: Theme.lightMode ? "Dark Mode" : "Light Mode"
                onClicked: root.themeToggleClicked()
            }

            ControlButton {
                text: "History"
                onClicked: root.historyClicked()
            }
        }
    }

    ColumnLayout {
        visible: root.stacked
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: root.title
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                visible: appController.machineType.length > 0 || appController.deviceId.length > 0
                text: appController.machineType.length > 0
                      ? appController.machineType + " (" + (appController.deviceId.length > 0 ? appController.deviceId : "Waiting for UID") + ")"
                      : "Waiting for UID"
                color: Theme.muted
                font.pixelSize: 12
                elide: Text.ElideRight
                Layout.maximumWidth: 220
                Layout.alignment: Qt.AlignVCenter
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            ControlButton {
                text: appController.connected ? "Disconnect" : "Connect"
                primary: appController.connected
                onClicked: root.connectionToggled()
            }

            ControlButton {
                text: Theme.lightMode ? "Dark Mode" : "Light Mode"
                onClicked: root.themeToggleClicked()
            }

            ControlButton {
                text: "History"
                onClicked: root.historyClicked()
            }
        }
    }
}
