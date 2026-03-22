import QtQuick
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    property string title: "Edge Maintenance Monitor"
    property bool compact: width < 1280

    signal connectionToggled()
    signal exportCsvClicked()
    signal historyClicked()
    signal themeToggleClicked()

    height: 72
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    RowLayout {
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
            visible: dataModel.machineType.length > 0 || dataModel.deviceId.length > 0
            text: dataModel.machineType.length > 0
                  ? dataModel.machineType + " (" + (dataModel.deviceId.length > 0 ? dataModel.deviceId : "Waiting for UID") + ")"
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
                text: dataModel.connected ? "Disconnect" : "Connect"
                primary: dataModel.connected
                onClicked: root.connectionToggled()
            }

            ControlButton {
                text: Theme.lightMode ? "Dark Mode" : "Light Mode"
                onClicked: root.themeToggleClicked()
            }

            ControlButton {
                text: "Export CSV"
                primary: true
                enabled: dataModel.csvFilePath.length > 0
                onClicked: root.exportCsvClicked()
            }

            ControlButton {
                text: dataModel.loggingEnabled ? "Stop Logging" : "Start Logging"
                primary: dataModel.loggingEnabled
                enabled: dataModel.connected || dataModel.loggingEnabled
                onClicked: dataModel.loggingEnabled ? dataModel.stopLogging() : dataModel.startLogging()
            }

            ControlButton {
                text: "History"
                onClicked: root.historyClicked()
            }
        }
    }
}
