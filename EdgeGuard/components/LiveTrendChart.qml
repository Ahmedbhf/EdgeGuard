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
    property real calculatedMaxY: 1.0
    property bool anomalyActive: false
    property bool showUnitLabel: true

    readonly property bool isDarkMode: !Theme.lightMode
    readonly property color effectiveLineColor: anomalyActive ? "#EF4444" : lineColor
    readonly property color lineGlowColor: Qt.rgba(effectiveLineColor.r, effectiveLineColor.g, effectiveLineColor.b, 0.16)
    readonly property color horizontalGridColor: isDarkMode ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.08)
    readonly property color verticalGridColor: isDarkMode ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.06)
    readonly property color axisTextColor: isDarkMode ? "#aeb6c2" : "#7b8794"
    readonly property color latestGlowColor: Qt.rgba(effectiveLineColor.r, effectiveLineColor.g, effectiveLineColor.b, 0.20)
    readonly property real effectiveMaxY: fixedMaxY > 0 ? fixedMaxY : calculatedMaxY
    readonly property real totalTimeSec: displayPoints / Math.max(1, sampleRateHz)
    readonly property real secondsPerStep: totalTimeSec / Math.max(1, displayPoints - 1)
    readonly property real currentValue: values && values.length > 0 ? values[values.length - 1] : 0
    readonly property real xTickStep: chooseStep(totalTimeSec, [0.5, 1, 2, 5, 10, 15, 30, 60])
    readonly property real yTickStep: chooseStep(Math.max(0.5, effectiveMaxY - fixedMinY), [0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50])
    readonly property string xLabelFormat: xTickStep < 1 ? "%.1f" : "%.0f"
    readonly property string yLabelFormat: yTickStep < 1 ? "%.1f" : "%.0f"

    function chooseStep(range, candidates) {
        var desiredStep = range / 5
        for (var i = 0; i < candidates.length; ++i) {
            if (candidates[i] >= desiredStep)
                return candidates[i]
        }

        return candidates[candidates.length - 1]
    }

    function updateDynamicMax() {
        // Pick a readable Y range automatically so the chart is easy to scan during live updates.
        calculatedMaxY = ChartUtils.computeDynamicMax(values)
    }

    function rebuildSeries() {
        // Rebuild all chart layers whenever the input values or visual settings change.
        updateDynamicMax()
        glowLineSeries.clear()
        trendSeries.clear()
        glowSeries.clear()
        markerSeries.clear()

        if (!values || values.length === 0)
            return

        var count = Math.min(values.length, displayPoints)
        var start = values.length - count

        // Plot from oldest to newest across the visible time window.
        for (var i = 0; i < count; i++) {
            var x = i * secondsPerStep
            glowLineSeries.append(x, values[start + i])
            trendSeries.append(x, values[start + i])
        }

        glowSeries.append((count - 1) * secondsPerStep, currentValue)
        markerSeries.append((count - 1) * secondsPerStep, currentValue)
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
        margins.top: 6
        margins.left: 6
        margins.right: 6
        margins.bottom: 6

        ValueAxis {
            id: axisX
            min: 0
            max: root.totalTimeSec
            tickType: ValueAxis.TicksDynamic
            tickAnchor: 0
            tickInterval: root.xTickStep
            labelFormat: root.xLabelFormat
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
            titleText: "Time (s)"
            gridVisible: true
            gridLineColor: root.verticalGridColor
            lineVisible: true
            linePenColor: root.verticalGridColor
            shadesVisible: false
        }

        ValueAxis {
            id: axisY
            min: root.fixedMinY
            max: root.effectiveMaxY
            tickType: ValueAxis.TicksDynamic
            tickAnchor: root.fixedMinY
            tickInterval: root.yTickStep
            labelFormat: root.yLabelFormat
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
            titleText: root.unit.length > 0 ? root.unit : "Value"
            gridVisible: true
            gridLineColor: root.horizontalGridColor
            minorGridVisible: false
            lineVisible: true
            linePenColor: root.horizontalGridColor
            shadesVisible: false
        }

        LineSeries {
            id: glowLineSeries
            axisX: axisX
            axisY: axisY
            color: root.lineGlowColor
            width: 5.0
        }

        LineSeries {
            id: trendSeries
            axisX: axisX
            axisY: axisY
            color: root.effectiveLineColor
            width: 2.2
        }

        ScatterSeries {
            id: glowSeries
            // A soft larger point helps the latest sample stand out.
            axisX: axisX
            axisY: axisY
            color: root.latestGlowColor
            borderColor: "transparent"
            markerSize: 16
        }

        ScatterSeries {
            id: markerSeries
            axisX: axisX
            axisY: axisY
            color: root.effectiveLineColor
            borderColor: "transparent"
            markerSize: 6
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

    onValuesChanged: rebuildSeries()
    onDisplayPointsChanged: rebuildSeries()
    onSampleRateHzChanged: rebuildSeries()
    onFixedMinYChanged: rebuildSeries()
    onFixedMaxYChanged: rebuildSeries()
    onAnomalyActiveChanged: rebuildSeries()
    onLineColorChanged: rebuildSeries()
    onShowUnitLabelChanged: rebuildSeries()
    Component.onCompleted: rebuildSeries()
}
