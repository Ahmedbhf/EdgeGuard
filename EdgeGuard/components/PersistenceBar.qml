import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Item {
    id: root

    property int value: 0
    property int maxValue: 5

    implicitHeight: 52

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Persistence"
                color: Theme.text
                font.pixelSize: 12
            }

            Item { Layout.fillWidth: true }

            Label {
                text: value + " / " + maxValue
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 8
            radius: 4
            color: Theme.panel2
            border.color: Theme.borderSoft
            border.width: 1

            Rectangle {
                height: parent.height
                radius: 4
                width: maxValue > 0 ? parent.width * (value / maxValue) : 0
                color: Theme.panel2
            }
        }
    }
}
