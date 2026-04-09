import QtQuick
import QtQuick.Layouts
import "../utils/HistoryChartUtils.js" as HistoryChartUtils
import EdgeGuard

Item {
    id: root

    // This wrapper keeps all history charts using the same time window and interactions.
    property var anomalyPoints: []
    property real anomalyMinY: 0
    property real anomalyMaxY: 100
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
            id: anomalyPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
            chartTitle: "Anomaly Score vs Time"
            valueLabel: "Score"
            valueFormat: "%.1f"
            valueDecimals: 1
            lineColor: Theme.text
            thresholdBands: [
                { from: 0, to: 50, color: "#EF4444", opacity: 0.16 },
                { from: 50, to: 80, color: "#F59E0B", opacity: 0.14 },
                { from: 80, to: 100, color: "#22C55E", opacity: 0.14 }
            ]
            points: root.anomalyPoints
            axisMinY: root.anomalyMinY
            axisMaxY: root.anomalyMaxY
            fullStartMs: root.fullStartMs
            fullEndMs: root.fullEndMs
            minimumWindowMs: root.minimumWindowMs
            interactiveEnabled: root.interactiveEnabled
            onVisibleRangeChanged: function(startMs, endMs) {
                if (root.synchronizingRange)
                    return

                root.synchronizingRange = true
                HistoryChartUtils.syncVisibleRange(startMs, endMs, anomalyPanel, [anomalyPanel, rmsPanel, tempPanel])
                root.synchronizingRange = false
            }
        }

        HistoryChartPanel {
            id: rmsPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
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
                HistoryChartUtils.syncVisibleRange(startMs, endMs, rmsPanel, [anomalyPanel, rmsPanel, tempPanel])
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
                HistoryChartUtils.syncVisibleRange(startMs, endMs, tempPanel, [anomalyPanel, rmsPanel, tempPanel])
                root.synchronizingRange = false
            }
        }
    }
}
