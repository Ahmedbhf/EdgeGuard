import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import EdgeGuard

Rectangle {
    id: root
    radius: Theme.radiusLg
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    property var values: []

    implicitHeight: 260
    Layout.fillWidth: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        // ===== HEADER =====
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Vibration"
                color: Theme.text
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Label {
                text: values.length + " / 240 points"
                color: Theme.muted
                font.pixelSize: 12
            }
        }

        // ===== CHART =====
        ChartView {
            id: chart
            Layout.fillWidth: true
            Layout.fillHeight: true

            backgroundColor: "transparent"
            legend.visible: false
            antialiasing: true

            ValueAxis {
                id: axisX
                min: 0
                max: 240
                visible: false
            }

            ValueAxis {
                id: axisY
                min: 0.8
                max: 1.3
                labelsColor: Theme.muted
                gridLineColor: Theme.borderSoft
                lineVisible: false
            }

            LineSeries {
                id: series
                axisX: axisX
                axisY: axisY
                color: "#E6EDF3"
                width: 2
            }

            Component.onCompleted: updateSeries()
        }
    }

    function updateSeries() {
        series.clear()
        for (var i = 0; i < values.length; i++) {
            series.append(i, values[i])
        }
    }

    onValuesChanged: updateSeries()
}
