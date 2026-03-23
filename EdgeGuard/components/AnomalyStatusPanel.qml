import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

PanelCard {
    id: root
    implicitHeight: 420
    title: "Anomaly Status"

    property var lastSampleTime: null
    property int refreshTick: 0
    readonly property string lastUpdateText: {
        refreshTick
        if (!lastSampleTime)
            return "Waiting for data"

        var seconds = Math.floor((new Date().getTime() - lastSampleTime.getTime()) / 1000)
        if (seconds <= 1)
            return "Just now"

        return seconds + " s ago"
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.refreshTick++
    }

    Connections {
        target: dataModel

        function onDataChanged() {
            root.lastSampleTime = new Date()
            root.refreshTick = 0
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceLg

        ValueCard {
            Layout.fillWidth: true
            label: "Anomaly Score"
            value: dataModel.anomalyScore.toFixed(2)
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
                    text: root.lastUpdateText
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
                    text: "OK"
                    active: dataModel.state === "OK"
                    activeColor: Theme.ok
                    borderHighlight: 1.2
                }

                StatusTile {
                    text: "ANOMALY"
                    active: dataModel.state === "ANOMALY"
                    activeColor: Theme.fault
                }
            }
        }
    }
}
