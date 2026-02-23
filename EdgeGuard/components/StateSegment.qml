import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."
import EdgeGuard

Item {
    id: root
    implicitHeight: 60

    property string state: "OK"   // OK | WARNING | FAULT
    property bool clickable: false
    signal statePicked(string newState)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "State"
            color: Theme.text
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            height: 44
            radius: 14
            color: Theme.panel2
            border.color: Theme.borderSoft
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                SegmentPill {
                    Layout.fillWidth: true
                    text: "OK"
                    active: root.state === "OK"
                    activeTextColor: Theme.text
                    onClicked: {
                        if (!root.clickable) return
                        root.state = "OK"
                        root.statePicked(root.state)
                    }
                }

                SegmentPill {
                    Layout.fillWidth: true
                    text: "WARNING"
                    active: root.state === "WARNING"
                    activeTextColor: Theme.text
                    onClicked: {
                        if (!root.clickable) return
                        root.state = "WARNING"
                        root.statePicked(root.state)
                    }
                }

                SegmentPill {
                    Layout.fillWidth: true
                    text: "FAULT"
                    active: root.state === "FAULT"
                    activeTextColor: Theme.text
                    onClicked: {
                        if (!root.clickable) return
                        root.state = "FAULT"
                        root.statePicked(root.state)
                    }
                }
            }
        }
    }
}
