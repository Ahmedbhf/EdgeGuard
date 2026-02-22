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
        radius: 20
        color: Theme.panel
        border.color: Theme.border

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ===== Header =====
            Rectangle {
                Layout.fillWidth: true
                height: 48
                color: "transparent"

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    text: "Anomaly & State"
                    color: "white"
                    font.bold: true
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.border
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    MetricCard {
                        Layout.fillWidth: true
                        label: "Z-score (received)"
                        value: "1.49"
                        sizeVariant: "large"

                    }

                    MetricCard {
                        Layout.fillWidth: true

                        label: "Threshold (received)"
                        value: "1.17"
                        sizeVariant: "large"
                    }

                    MetricCard {
                        Layout.fillWidth: true

                        label: "Window"
                        value: "6200"
                        sizeVariant: "large"
                    }
                    PersistenceBar {
                         Layout.fillHeight: true
                        Layout.fillWidth: true
                        value: persistence
                        maxValue: 5
                    }

                    StateSegment {
                         Layout.fillHeight: true
                        Layout.fillWidth: true
                        state: root.state
                        clickable: false
                    }
                }
            }
        }
    }
}
