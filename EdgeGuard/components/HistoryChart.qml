import QtQuick
import QtQuick.Layouts
import EdgeGuard

Item {
    id: root

    // This wrapper keeps both history charts using the same time window and interactions.
    property var rmsPoints: []
    property var tempPoints: []
    property real rmsMinY: 0
    property real rmsMaxY: 1
    property real tempMinY: 0
    property real tempMaxY: 1
    property real viewStartMs: 0
    property real viewEndMs: 1000
    property bool interactiveEnabled: false

    signal panRequested(real pixelDelta, real chartWidth)
    signal zoomRequested(real factor)

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        HistoryChartPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
            // RMS and temperature charts share the same pan and zoom controls from the parent page.
            chartTitle: "RMS vs Time"
            valueLabel: "RMS"
            valueFormat: "%.2f"
            valueDecimals: 2
            lineColor: Theme.primary
            points: root.rmsPoints
            axisMinY: root.rmsMinY
            axisMaxY: root.rmsMaxY
            viewStartMs: root.viewStartMs
            viewEndMs: root.viewEndMs
            interactiveEnabled: root.interactiveEnabled
            onPanRequested: function(pixelDelta, chartWidth) { root.panRequested(pixelDelta, chartWidth) }
            onZoomRequested: function(factor) { root.zoomRequested(factor) }
        }

        HistoryChartPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
            chartTitle: "Temperature vs Time"
            valueLabel: "Temp"
            valueFormat: "%.1f"
            valueDecimals: 1
            lineColor: "#F59E0B"
            points: root.tempPoints
            axisMinY: root.tempMinY
            axisMaxY: root.tempMaxY
            viewStartMs: root.viewStartMs
            viewEndMs: root.viewEndMs
            interactiveEnabled: root.interactiveEnabled
            onPanRequested: function(pixelDelta, chartWidth) { root.panRequested(pixelDelta, chartWidth) }
            onZoomRequested: function(factor) { root.zoomRequested(factor) }
        }
    }
}
