import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

PanelCard {
    id: root
    implicitHeight: 420
    title: "Anomaly Status"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceLg

        ValueCard {
            Layout.fillWidth: true
            label: "Score"
            value: appController.anomalyScore.toFixed(2)
            sizeVariant: "large"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            radius: Theme.radiusMd
            color: Theme.panel2
            border.color: Theme.borderSoft
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceLg
                spacing: Theme.spaceSm

                Label {
                    text: "Last Update"
                    color: Theme.muted
                    font.pixelSize: 12
                }

                Text {
                    Layout.fillWidth: true
                    text: appController.lastUpdateText
                    color: Theme.text
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 150
            radius: Theme.radiusMd
            color: Theme.panel2
            border.color: Theme.borderSoft
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceLg
                spacing: Theme.spaceMd

                Label {
                    text: "Current State"
                    color: Theme.muted
                    font.pixelSize: 12
                }

                StatusTile {
                    text: "NORMAL"
                    active: appController.state === "NORMAL"
                    activeColor: Theme.ok
                    borderHighlight: 1.2
                }

                StatusTile {
                    text: "WARNING"
                    active: appController.state === "WARNING"
                    activeColor: Theme.warning
                }

                StatusTile {
                    text: "CRITICAL"
                    active: appController.state === "CRITICAL"
                    activeColor: Theme.fault
                }
            }
        }
    }
}
