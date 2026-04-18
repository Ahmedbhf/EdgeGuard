import QtCharts
import QtQuick
import "../utils/ChartUtils.js" as ChartUtils
import EdgeGuard

Rectangle {
    id: root

    color: "transparent"
    border.width: 0
    clip: true

    property var values: []
    property string unit: ""
    property int displayPoints: 60
    property color lineColor: Theme.primary
    property real sampleRateHz: 10.0
    property real fixedMinY: 0.0
    property real fixedMaxY: -1
    property real calculatedMinY: 0.0
    property real calculatedMaxY: 1.0
    property bool anomalyActive: false
    property bool showUnitLabel: true
    property bool clampMinYToZero: true

    readonly property bool isDarkMode: !Theme.lightMode
    readonly property color effectiveLineColor: anomalyActive ? "#EF4444" : lineColor
    readonly property color lineGlowColor: "transparent"
    readonly property color horizontalGridColor: isDarkMode ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.08)
    readonly property color verticalGridColor: isDarkMode ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.06)
    readonly property color axisTextColor: isDarkMode ? "#aeb6c2" : "#7b8794"
    readonly property color latestGlowColor: "transparent"
    readonly property bool hasFixedRange: fixedMaxY > fixedMinY
    readonly property real effectiveMinY: hasFixedRange ? fixedMinY : calculatedMinY
    readonly property real effectiveMaxY: hasFixedRange ? fixedMaxY : calculatedMaxY
    readonly property real totalTimeSec: displayPoints / Math.max(1, sampleRateHz)
    readonly property real secondsPerStep: totalTimeSec / Math.max(1, displayPoints - 1)
    readonly property real currentValue: values && values.length > 0 ? values[values.length - 1] : 0
    readonly property string updateRateText: {
        var intervalMs = 1000 / Math.max(0.001, sampleRateHz)
        if (intervalMs >= 1000)
            return "Chart updates every " + (intervalMs / 1000).toFixed(intervalMs % 1000 === 0 ? 0 : 1) + " s"
        return "Chart updates every " + Math.round(intervalMs) + " ms"
    }

    function updateDynamicRange() {
        // Pick a readable Y range automatically so the chart is easy to scan during live updates.
        var range = ChartUtils.computeRange(values, clampMinYToZero)
        calculatedMinY = range.min
        calculatedMaxY = range.max
    }

    function rebuildSeries() {
        // Rebuild all chart layers whenever the input values or visual settings change.
        updateDynamicRange()
        glowLineSeries.clear()
        trendSeries.clear()
        glowSeries.clear()
        markerSeries.clear()

        if (!values || values.length === 0)
            return

        var count = Math.min(values.length, displayPoints)
        var start = values.length - count

        // Keep the newest point anchored at time 0 on the right edge.
        for (var i = 0; i < count; i++) {
            var x = -((count - 1 - i) * secondsPerStep)
            glowLineSeries.append(x, values[start + i])
            trendSeries.append(x, values[start + i])
        }

        glowSeries.append(0, currentValue)
        markerSeries.append(0, currentValue)
    }

    ChartView {
        id: chartView
        anchors.fill: parent
        anchors.margins: 0
        antialiasing: false
        legend.visible: false
        backgroundRoundness: 0
        backgroundColor: "transparent"
        plotAreaColor: "transparent"
        margins.top: 0
        margins.left: 0
        margins.right: 0
        margins.bottom: 14

        ValueAxis {
            id: axisX
            min: -root.totalTimeSec
            max: 0
            tickCount: 6
            labelFormat: ""
            labelsColor: "transparent"
            labelsFont.pixelSize: 10
            gridVisible: true
            gridLineColor: root.verticalGridColor
            lineVisible: false
            shadesVisible: false
        }

        ValueAxis {
            id: axisY
            min: root.effectiveMinY
            max: root.effectiveMaxY
            tickCount: 5
            labelFormat: root.effectiveMaxY - root.effectiveMinY >= 10 ? "%.0f" : "%.1f"
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
            gridVisible: true
            gridLineColor: root.horizontalGridColor
            minorGridVisible: false
            lineVisible: false
            shadesVisible: false
        }

        LineSeries {
            id: glowLineSeries
            axisX: axisX
            axisY: axisY
            color: root.lineGlowColor
            width: 0
        }

        LineSeries {
            id: trendSeries
            axisX: axisX
            axisY: axisY
            color: root.effectiveLineColor
            width: 2.8
        }

        ScatterSeries {
            id: glowSeries
            // A soft larger point helps the latest sample stand out.
            axisX: axisX
            axisY: axisY
            color: root.latestGlowColor
            borderColor: "transparent"
            markerSize: 0
        }

        ScatterSeries {
            id: markerSeries
            axisX: axisX
            axisY: axisY
            color: root.effectiveLineColor
            borderColor: "transparent"
            markerSize: 4
        }
    }

    Text {
        visible: root.showUnitLabel && root.unit.length > 0
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 2
        anchors.bottomMargin: 2
        text: root.unit
        color: root.axisTextColor
        font.pixelSize: 10
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        text: root.updateRateText
        color: root.axisTextColor
        opacity: 0.7
        font.pixelSize: 10
    }

    onValuesChanged: rebuildSeries()
    onDisplayPointsChanged: rebuildSeries()
    onSampleRateHzChanged: rebuildSeries()
    onFixedMinYChanged: rebuildSeries()
    onFixedMaxYChanged: rebuildSeries()
    onAnomalyActiveChanged: rebuildSeries()
    onClampMinYToZeroChanged: rebuildSeries()
    onLineColorChanged: rebuildSeries()
    onShowUnitLabelChanged: rebuildSeries()
    Component.onCompleted: rebuildSeries()
}
