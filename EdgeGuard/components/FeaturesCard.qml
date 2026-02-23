import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true   //  RowLayout
    radius: 20
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    implicitHeight: 420

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== TITLE =====
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "transparent"

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                text: "Features"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.text
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Qt.rgba(1,1,1,0.06)
            }
        }

        // ===== CONTENT =====
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 24

                // ===== MAIN METRICS =====
                GridLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    columns: 2
                    columnSpacing: 18
                    rowSpacing: 18

                    MetricCard { Layout.fillWidth: true; label: "RMS (g)"; value: dataModel.rms.toFixed(2) }
                    MetricCard { Layout.fillWidth: true; label: "Peak2Peak (g)"; value: dataModel.peak2peak.toFixed(2) }
                    MetricCard { Layout.fillWidth: true; label: "Variance"; value: dataModel.variance.toFixed(2) }
                    MetricCard { Layout.fillWidth: true; label: "Crest Factor"; value: dataModel.crestFactor.toFixed(2) }
                    MetricCard { Layout.fillWidth: true; label: "Temp (°C)"; value: dataModel.temp.toFixed(1) }
                    MetricCard { Layout.fillWidth: true; label: "Temp Slope (°C/min)"; value: dataModel.tempSlope.toFixed(2) }
                }

                // ===== DIVIDER =====
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1,1,1,0.05)
                }


                // ===== BASELINE SECTION =====
                Text {
                    text: "Baseline (display only)"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: Theme.text
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 18
                    rowSpacing: 18

                    MetricCard { Layout.fillWidth: true; label: "μ RMS"; value: "1.10" }
                    MetricCard { Layout.fillWidth: true; label: "σ RMS"; value: "0.15" }
                    MetricCard { Layout.fillWidth: true; label: "μ Temp"; value: "40.00" }
                    MetricCard { Layout.fillWidth: true; label: "σ Temp"; value: "1.20" }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
