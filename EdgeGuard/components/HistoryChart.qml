import QtQuick
import QtQuick.Layouts
import "../utils/HistoryChartUtils.js" as HistoryChartUtils
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
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real minimumWindowMs: 1000
    property bool interactiveEnabled: false

    property bool synchronizingRange: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        HistoryChartPanel {
            id: rmsPanel
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
            fullStartMs: root.fullStartMs
            fullEndMs: root.fullEndMs
            minimumWindowMs: root.minimumWindowMs
            interactiveEnabled: root.interactiveEnabled
            onVisibleRangeChanged: function(startMs, endMs) {
                if (root.synchronizingRange)
                    return

                root.synchronizingRange = true
                HistoryChartUtils.syncVisibleRange(startMs, endMs, rmsPanel, rmsPanel, tempPanel)
                root.synchronizingRange = false
            }
        }

        HistoryChartPanel {
            id: tempPanel
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
            fullStartMs: root.fullStartMs
            fullEndMs: root.fullEndMs
            minimumWindowMs: root.minimumWindowMs
            interactiveEnabled: root.interactiveEnabled
            onVisibleRangeChanged: function(startMs, endMs) {
                if (root.synchronizingRange)
                    return

                root.synchronizingRange = true
                HistoryChartUtils.syncVisibleRange(startMs, endMs, tempPanel, rmsPanel, tempPanel)
                root.synchronizingRange = false
            }
        }
    }
}
