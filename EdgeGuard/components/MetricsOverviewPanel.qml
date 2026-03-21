import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 16
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    implicitHeight: 420

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                text: "Key Metrics"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.text
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.borderSoft
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                ValueCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    Layout.verticalStretchFactor: 1
                    label: "RMS (mg)"
                    value: dataModel.rms.toFixed(2)
                    sizeVariant: "hero"
                }

                ValueCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    Layout.verticalStretchFactor: 1
                    label: "Temperature (°C)"
                    value: dataModel.temp.toFixed(1)
                    sizeVariant: "hero"
                }
            }
        }
    }
}
