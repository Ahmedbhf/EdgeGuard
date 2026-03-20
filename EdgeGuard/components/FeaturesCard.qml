import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 16
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    implicitHeight: 420

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

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
                color: Theme.borderSoft
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                anchors.fill: parent
                anchors.margins: 16
                columns: 2
                rowSpacing: 16
                columnSpacing: 16

                MetricCard { Layout.fillWidth: true; Layout.fillHeight: true; label: "RMS (mg)"; value: dataModel.rms.toFixed(2); sizeVariant: "feature" }
                MetricCard { Layout.fillWidth: true; Layout.fillHeight: true; label: "Temp (°C)"; value: dataModel.temp.toFixed(1); sizeVariant: "feature" }
                MetricCard { Layout.fillWidth: true; Layout.fillHeight: true; label: "Peak2Peak (g)"; value: dataModel.peak2peak.toFixed(2); sizeVariant: "feature" }
                MetricCard { Layout.fillWidth: true; Layout.fillHeight: true; label: "Variance"; value: dataModel.variance.toFixed(2); sizeVariant: "feature" }
                MetricCard { Layout.fillWidth: true; Layout.fillHeight: true; label: "Crest Factor"; value: dataModel.crestFactor.toFixed(2); sizeVariant: "feature" }
                MetricCard { Layout.fillWidth: true; Layout.fillHeight: true; label: "Temp Slope (°C/min)"; value: dataModel.tempSlope.toFixed(2); sizeVariant: "feature" }
            }
        }
    }
}
