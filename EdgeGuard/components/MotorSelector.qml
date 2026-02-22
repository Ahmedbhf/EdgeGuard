import QtQuick
import QtQuick.Controls
import EdgeGuard

Rectangle {
    id: root

    property int currentIndex: 0
    signal motorChanged(int index)

    height: 64
    radius: 18
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    // INNER SEGMENTED CONTAINER
    Rectangle {
        id: inner
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 16

        height: 40
        radius: 14
        color: Theme.panel
        border.color: Theme.border
        border.width: 1

        implicitWidth: row.implicitWidth + 8

        Row {
            id: row
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            Repeater {
                model: ["Motor A", "Motor B"]

                delegate: Rectangle {
                    required property int index
                    required property string modelData

                    height: parent.height
                    radius: 10
                    implicitWidth: label.implicitWidth + 32

                    color: root.currentIndex === index
                           ? Theme.panel2
                           : "transparent"

                    border.width: root.currentIndex === index ? 1 : 0
                    border.color: Theme.border

                    Behavior on color {
                        ColorAnimation { duration: 120 }
                    }

                    Text {
                        id: label
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 14
                        font.bold: true
                        color: root.currentIndex === index
                               ? Theme.text
                               : Theme.muted
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.currentIndex = index
                            root.motorChanged(index)
                        }
                    }
                }
            }
        }
    }
}
