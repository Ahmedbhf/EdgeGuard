import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."
import EdgeGuard

Item {
    id: root
    implicitHeight: 60

    property string state: "OK"
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
            height: 48
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
                    activeFillColor: Theme.panel
                    activeBorderColor: Theme.ok
                    activeTextColor: Theme.ok
                    onClicked: {
                        if (!root.clickable) return
                        root.state = "OK"
                        root.statePicked(root.state)
                    }
                }

                SegmentPill {
                    Layout.fillWidth: true
                    text: "ANOMALY"
                    active: root.state === "ANOMALY"
                    activeFillColor: Theme.panel
                    activeBorderColor: Theme.warning
                    activeTextColor: Theme.warning
                    onClicked: {
                        if (!root.clickable) return
                        root.state = "ANOMALY"
                        root.statePicked(root.state)
                    }
                }
            }
        }
    }
}
