import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Item {
    id: root
    implicitHeight: 420

    property real zScore: 0.0
    property real threshold: 3.0
    property int persistence: 0
    property string state: "OK"

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.panel
        border.color: Theme.border
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ===== Header =====
            Rectangle {
                Layout.fillWidth: true
                height: 48
                color: "transparent"
                radius:16

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spaceLg
                    text: "Anomaly & State"
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
                    spacing: Theme.spaceMd

                    MetricCard {
                        Layout.fillWidth: true
                        label: "Z-score (received)"
                        value: dataModel.zScore.toFixed(2)
                        sizeVariant: "large"
                    }

                    MetricCard {
                        Layout.fillWidth: true
                        label: "Threshold (received)"
                        value: threshold.toFixed(2)
                        sizeVariant: "large"
                    }

                    MetricCard {
                        Layout.fillWidth: true
                        label: "Window"
                        value: "6200"
                        sizeVariant: "large"
                    }

                    PersistenceBar {
                        Layout.fillWidth: true
                        value: persistence
                        maxValue: 5
                    }

                    StateSegment {
                        Layout.fillWidth: true
                        state: root.state
                        clickable: false
                    }
                }
            }
        }
    }
}
