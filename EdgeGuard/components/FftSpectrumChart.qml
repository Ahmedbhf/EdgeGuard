import QtCharts
import QtQuick
import "../utils/ChartUtils.js" as ChartUtils
import EdgeGuard

Rectangle {
    id: root

    color: "transparent"
    border.width: 0
    clip: true

    property var frequencies: []
    property var magnitudes: []
    property color lineColor: Theme.primary
    property real dominantFrequency: 0.0
    property real calculatedMaxY: 1.0

    readonly property bool isDarkMode: !Theme.lightMode
    readonly property color horizontalGridColor: isDarkMode ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.08)
    readonly property color verticalGridColor: isDarkMode ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.06)
    readonly property color axisTextColor: isDarkMode ? "#aeb6c2" : "#7b8794"
    readonly property real effectiveMaxX: frequencies && frequencies.length > 0 ? frequencies[frequencies.length - 1] : 1.0
    readonly property real effectiveMaxY: Math.max(1.0, calculatedMaxY)

    function rebuildSeries() {
        var range = ChartUtils.computeRange(magnitudes, true)
        calculatedMaxY = Math.max(1.0, range.max)

        spectrumSeries.clear()
        dominantMarker.clear()

        if (!frequencies || !magnitudes || frequencies.length === 0 || magnitudes.length === 0)
            return

        var count = Math.min(frequencies.length, magnitudes.length)
        var dominantMagnitude = 0.0
        for (var i = 0; i < count; ++i) {
            spectrumSeries.append(frequencies[i], magnitudes[i])
            if (Math.abs(frequencies[i] - dominantFrequency) < 0.0001)
                dominantMagnitude = magnitudes[i]
        }

        dominantMarker.append(dominantFrequency, dominantMagnitude)
    }

    ChartView {
        anchors.fill: parent
        anchors.margins: 0
        antialiasing: false
        legend.visible: false
        backgroundRoundness: 0
        backgroundColor: "transparent"
        plotAreaColor: "transparent"
        margins.top: 8
        margins.left: 0
        margins.right: 0
        margins.bottom: 18

        ValueAxis {
            id: axisX
            min: 0
            max: root.effectiveMaxX
            tickCount: 6
            labelFormat: root.effectiveMaxX >= 10 ? "%.0f" : "%.1f"
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
            gridVisible: true
            gridLineColor: root.verticalGridColor
            lineVisible: false
            shadesVisible: false
        }

        ValueAxis {
            id: axisY
            min: 0
            max: root.effectiveMaxY
            tickCount: 5
            labelFormat: root.effectiveMaxY >= 10 ? "%.0f" : "%.1f"
            labelsColor: root.axisTextColor
            labelsFont.pixelSize: 10
            gridVisible: true
            gridLineColor: root.horizontalGridColor
            lineVisible: false
            shadesVisible: false
        }

        LineSeries {
            id: spectrumSeries
            axisX: axisX
            axisY: axisY
            color: root.lineColor
            width: 2.4
        }

        ScatterSeries {
            id: dominantMarker
            axisX: axisX
            axisY: axisY
            color: Theme.primary
            borderColor: "transparent"
            markerSize: 8
        }
    }

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 2
        anchors.topMargin: 0
        text: "Amplitude"
        color: root.axisTextColor
        font.pixelSize: 10
    }

    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 2
        anchors.bottomMargin: 0
        text: "Frequency (Hz)"
        color: root.axisTextColor
        font.pixelSize: 10
    }

    onFrequenciesChanged: rebuildSeries()
    onMagnitudesChanged: rebuildSeries()
    onDominantFrequencyChanged: rebuildSeries()
    onLineColorChanged: rebuildSeries()
    Component.onCompleted: rebuildSeries()
}
