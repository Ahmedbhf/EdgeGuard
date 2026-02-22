import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

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
            color: "#C9D1D9"
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            height: 44
            radius: 14
            color: Qt.rgba(1,1,1,0.03)
            border.color: Qt.rgba(1,1,1,0.10)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                SegmentPill {
                    Layout.fillWidth: true
                    text: "OK"
                    active: root.state === "OK"
                    activeTextColor: "#9BE9A8"
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
                    activeTextColor: "#FFD580"
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
                    activeTextColor: "#FF7B72"
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
