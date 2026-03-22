import QtCharts
import QtQuick
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
    readonly property color axisTextColor: isDarkMode ? "#aeb6c2" : "#7b8794"
    readonly property color latestGlowColor: Qt.rgba(effectiveLineColor.r, effectiveLineColor.g, effectiveLineColor.b, 0.20)
    readonly property real effectiveMaxY: fixedMaxY > 0 ? fixedMaxY : calculatedMaxY
    readonly property real totalTimeSec: displayPoints / Math.max(1, sampleRateHz)
    readonly property real secondsPerStep: totalTimeSec / Math.max(1, displayPoints - 1)
    readonly property real currentValue: values && values.length > 0 ? values[values.length - 1] : 0

    function updateDynamicMax() {
        if (!values || values.length === 0) {
            calculatedMaxY = 1.0
            return
        }

        var max = values[0]
        for (var i = 1; i < values.length; i++) {
            if (values[i] > max)
                max = values[i]
        }

        var padded = max * 1.2
        if (padded <= 1) padded = 1
        else if (padded <= 2) padded = 2
        else if (padded <= 5) padded = 5
        else if (padded <= 10) padded = 10
        else if (padded <= 20) padded = 20
        else if (padded <= 50) padded = 50
        else if (padded <= 100) padded = 100
        else if (padded <= 200) padded = 200
        else padded = Math.ceil(padded / 50) * 50

        calculatedMaxY = padded
    }

    function rebuildSeries() {
        updateDynamicMax()
        glowLineSeries.clear()
        trendSeries.clear()
        glowSeries.clear()
        markerSeries.clear()

        if (!values || values.length === 0)
            return

        var count = Math.min(values.length, displayPoints)
        var start = values.length - count

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
        margins.bottom: 0

        ValueAxis {
            id: axisX
            min: -root.totalTimeSec
            max: 0
            tickCount: root.totalTimeSec >= 20 ? 5 : 4
            labelFormat: "%.0fs"
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
            gridVisible: false
            lineVisible: false
            shadesVisible: false
        }

        ValueAxis {
            id: axisY
            min: root.fixedMinY
            max: root.effectiveMaxY
            tickCount: 3
            labelFormat: "%.1f"
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
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
