import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    property string title: "Edge Maintenance Monitor"
    property bool compact: width < 1100

    signal connectionToggled()
    signal exportCsvClicked()
    signal refreshClicked()
    signal themeToggleClicked()

    height: 64
    radius: 0
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: Theme.panel
        opacity: 0.8
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        Text {
            text: root.title
            color: Theme.text
            font.pixelSize: root.compact ? 16 : 18
            font.bold: true
            elide: Text.ElideRight
            Layout.maximumWidth: root.compact ? 220 : 360
        }

        Item {
            Layout.fillWidth: true

            Flow {
                anchors.centerIn: parent
                spacing: 8

                Chip {
                    text: dataModel.connected ? "UART: Connected (" + dataModel.currentPort + ")" : "UART: Disconnected"
                    stateType: dataModel.connected ? "ok" : "neutral"
                }
            }
        }

        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            ActionButton {
                text: "Refresh"
                onClicked: root.refreshClicked()
            }

            ActionButton {
                text: Theme.lightMode ? "Dark Mode" : "Light Mode"
                onClicked: root.themeToggleClicked()
            }

            ActionButton {
                text: root.compact ? "" : (dataModel.connected ? "Disconnect" : "Connect UART")
                enabled: dataModel.connected || dataModel.selectedPort.length > 0
                onClicked: root.connectionToggled()
            }

            ActionButton {
                text: "Export CSV"
                primary: true
                enabled: dataModel.csvFilePath.length > 0
                onClicked: root.exportCsvClicked()
            }
        }
    }
}
