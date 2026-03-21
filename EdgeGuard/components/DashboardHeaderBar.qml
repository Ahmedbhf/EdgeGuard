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
    signal refreshClicked()
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

        StatusBadge {
            text: dataModel.connected ? "UART: " + dataModel.currentPort : "UART: Disconnected"
            tone: dataModel.connected ? "ok" : "neutral"
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            ControlButton {
                text: "Refresh"
                onClicked: root.refreshClicked()
            }

            ControlButton {
                text: Theme.lightMode ? "Dark Mode" : "Light Mode"
                onClicked: root.themeToggleClicked()
            }

            ControlButton {
                text: dataModel.connected ? "Disconnect" : "Connect UART"
                enabled: dataModel.connected || dataModel.selectedPort.length > 0
                onClicked: root.connectionToggled()
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
