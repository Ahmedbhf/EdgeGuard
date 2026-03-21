import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root

    Layout.fillWidth: true
    Layout.preferredHeight: 220
    radius: 18
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

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
                text: "Event Log"
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

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            clip: true

            TextArea {
                text: dataModel.logText
                readOnly: true
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                color: Theme.text
                font.pixelSize: 13

                background: Rectangle {
                    radius: 12
                    color: Theme.panel2
                    border.color: Theme.borderSoft
                    border.width: 1
                }
            }
        }
    }
}
