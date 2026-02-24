import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true   //  RowLayout
    radius: 16
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    implicitHeight: 420

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== HEADER =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Label {
                    text: "Raw Signals"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: Theme.text
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }

        // ===== DIVIDER =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSoft
        }

        // ===== CONTENT AREA (fills remaining space) =====
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"


        }
    }
}
