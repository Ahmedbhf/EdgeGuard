import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Item {
    id: root
    implicitHeight: 420
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

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.panel
        border.color: Theme.border
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
                    anchors.leftMargin: Theme.spaceLg
                    text: "Anomaly Status"
                    color: Theme.text
                    font.bold: true
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

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 52
                                radius: 12
                                color: dataModel.state === "OK" ? Theme.ok : Theme.panel
                                border.color: dataModel.state === "OK" ? Qt.lighter(Theme.ok, 1.2) : Theme.borderSoft
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "OK"
                                    color: dataModel.state === "OK" ? Theme.primaryFg : Theme.muted
                                    opacity: dataModel.state === "OK" ? 1.0 : 0.7
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 52
                                radius: 12
                                color: dataModel.state === "ANOMALY" ? Theme.fault : Theme.panel
                                border.color: dataModel.state === "ANOMALY" ? Qt.lighter(Theme.fault, 1.15) : Theme.borderSoft
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "ANOMALY"
                                    color: dataModel.state === "ANOMALY" ? Theme.primaryFg : Theme.muted
                                    opacity: dataModel.state === "ANOMALY" ? 1.0 : 0.7
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
