import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard
Rectangle {
    id: root
    property string title: "Edge Maintenance Monitor"

    // status values (later from backend)
    property string uartStatus: "UART: Connected"
    property string wifiStatus: "WiFi Bridge: Off"
    property string sourceMode: "Source Mode: UART"

    // layout responsiveness
    property bool compact: width < 1100

    signal connectClicked()
    signal disconnectClicked()
    signal resetViewClicked()
    signal exportCsvClicked()

    height:  64
    radius: 0
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    // subtle top highlight
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

        // LEFT: title
        Text {
            text: root.title
            color: Theme.text
            font.pixelSize: root.compact ? 16 : 18
            font.bold: true
            elide: Text.ElideRight
            Layout.maximumWidth: root.compact ? 220 : 360
        }

        // MIDDLE: chips (wrap when narrow)
        Item {
            Layout.fillWidth: true

            Flow {
                anchors.centerIn: parent
                spacing: 8

                Chip {
                    text: root.uartStatus
                    stateType: "neutral"
                }

                Chip {
                    text: root.sourceMode
                    stateType: "neutral"
                }
            }
        }
        // RIGHT: actions
        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            // in compact mode, show icons only (still clickable)
            ActionButton {
                text: root.compact ? "" : "Connect UART"
                toolTip: "Connect UART"
                onClicked: root.connectClicked()
            }

            ActionButton {
                text: root.compact ? "" : "Disconnect"
                toolTip: "Disconnect"
                onClicked: root.disconnectClicked()
            }

            ActionButton {
                text: root.compact ? "" : "Reset View"
                toolTip: "Reset View"
                onClicked: root.resetViewClicked()
            }

            ActionButton {
                text: "Export CSV"
                primary: true
                onClicked: root.exportCsvClicked()
            }
        }
    }
}
