import QtQuick
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root

    color: Theme.panel
    radius: 8
    border.color: Theme.borderSoft
    border.width: 1
    clip: true

    // ===== EXISTING API =====
    property var values: []
    property string unit: ""
    property int displayPoints: 60
    property color lineColor: Theme.primary
    readonly property bool isDarkMode: !Theme.lightMode
    readonly property color effectiveLineColor: isDarkMode ? Qt.lighter(lineColor, 1.15) : Qt.darker(lineColor, 1.25)
    readonly property color chartBackgroundColor: isDarkMode ? Theme.panel3 : "#ffffff"
    readonly property color chartGridColor: isDarkMode ? "#2a2a2f" : "#d1d5db"
    readonly property color axisTextColor: isDarkMode ? Theme.text : "#374151"
    readonly property color chartBorderColor: isDarkMode ? Theme.borderSoft : "#d1d5db"

    // ===== Sample rate for time axis =====
    property real sampleRateHz: 10.0

    // ===== Y-axis: min fixed, max dynamic =====
    property real fixedMinY: 0.0
    property real fixedMaxY: -1        // -1 means auto-calculate
    property real calculatedMaxY: 1.0  // Computed from data

    // ===== Chart margins =====
    property int leftMargin: 56
    property int rightMargin: 16
    property int topMargin: 20
    property int bottomMargin: 28

    // ===== Track total samples =====
    property int discardedSamples: 0
    property int lastValueCount: 0

    // The actual max used for drawing
    readonly property real effectiveMaxY: {
        if (fixedMaxY > 0) return fixedMaxY
        return calculatedMaxY
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Immediate

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var chartLeft = root.leftMargin
            var chartRight = width - root.rightMargin
            var chartTop = root.topMargin
            var chartBottom = height - root.bottomMargin
            var chartWidth = chartRight - chartLeft
            var chartHeight = chartBottom - chartTop

            if (chartWidth <= 0 || chartHeight <= 0)
                return

            // Use fixed min + effective max
            var minY = root.fixedMinY
            var maxY = root.effectiveMaxY
            var rangeY = maxY - minY
            if (rangeY === 0) rangeY = 1

            var midY = minY + rangeY / 2.0
            var totalTimeSec = root.displayPoints / root.sampleRateHz

            // ============================
            // 1. BACKGROUND FILL
            // ============================
            ctx.fillStyle = root.chartBackgroundColor
            ctx.fillRect(chartLeft, chartTop, chartWidth, chartHeight)

            // ============================
            // 2. HORIZONTAL GRID LINES
            // ============================
            var hGridValues = [minY, midY, maxY]

            ctx.strokeStyle = root.chartGridColor
            ctx.lineWidth = 1

            for (var h = 0; h < hGridValues.length; h++) {
                var normalizedH = (hGridValues[h] - minY) / rangeY
                var gridY = chartBottom - (normalizedH * chartHeight)

                ctx.beginPath()
                ctx.setLineDash([2, 4])
                ctx.moveTo(chartLeft, gridY)
                ctx.lineTo(chartRight, gridY)
                ctx.stroke()
            }
            ctx.setLineDash([])

            // ============================
            // 3. VERTICAL GRID LINES (time)
            // ============================
            var timeStepSec = 1.0
            if (totalTimeSec > 12) timeStepSec = 3.0
            else if (totalTimeSec > 6) timeStepSec = 2.0

            var numTimeMarkers = Math.floor(totalTimeSec / timeStepSec)

            ctx.strokeStyle = root.chartGridColor
            ctx.lineWidth = 1

            for (var t = 0; t <= numTimeMarkers; t++) {
                var timeSec = t * timeStepSec
                var sampleIndex = timeSec * root.sampleRateHz
                var vx = chartLeft + (sampleIndex / root.displayPoints) * chartWidth

                if (vx >= chartLeft && vx <= chartRight) {
                    ctx.beginPath()
                    ctx.setLineDash([2, 4])
                    ctx.moveTo(vx, chartTop)
                    ctx.lineTo(vx, chartBottom)
                    ctx.stroke()
                }
            }
            ctx.setLineDash([])

            // ============================
            // 4. CHART BORDER
            // ============================
            ctx.strokeStyle = root.chartBorderColor
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.rect(chartLeft, chartTop, chartWidth, chartHeight)
            ctx.stroke()

            // ============================
            // 5. Y AXIS LABELS (left side with unit)
            // ============================
            ctx.fillStyle = root.axisTextColor
            ctx.font = "10px sans-serif"
            ctx.textAlign = "right"
            ctx.textBaseline = "middle"

            ctx.fillText(maxY.toFixed(1) + " " + root.unit, chartLeft - 6, chartTop)
            ctx.fillText(midY.toFixed(1) + " " + root.unit, chartLeft - 6, chartTop + chartHeight / 2)
            ctx.fillText(minY.toFixed(1) + " " + root.unit, chartLeft - 6, chartBottom)

            // ============================
            // 6. X AXIS LABELS (time)
            // ============================
            ctx.fillStyle = root.axisTextColor
            ctx.font = "10px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "top"

            for (var tx = 0; tx <= numTimeMarkers; tx++) {
                var tSec = tx * timeStepSec
                var tSampleIdx = tSec * root.sampleRateHz
                var labelX = chartLeft + (tSampleIdx / root.displayPoints) * chartWidth
                var labelTimeSec = -(totalTimeSec - tSec)

                var label = ""
                if (Math.abs(labelTimeSec) < 0.01) {
                    label = "Now"
                } else {
                    label = labelTimeSec.toFixed(0) + "s"
                }

                if (labelX >= chartLeft && labelX <= chartRight) {
                    ctx.fillText(label, labelX, chartBottom + 6)
                }
            }

            // ============================
            // 7. DRAW SIGNAL LINE
            // ============================
            if (!root.values || root.values.length === 0)
                return

            var start = Math.max(0, root.values.length - root.displayPoints)
            var count = Math.min(root.values.length, root.displayPoints)
            var stepX = chartWidth / Math.max(1, (root.displayPoints - 1))

            // Glow effect
            ctx.strokeStyle = Qt.rgba(
                root.effectiveLineColor.r,
                root.effectiveLineColor.g,
                root.effectiveLineColor.b,
                0.15
            )
            ctx.lineWidth = 6
            ctx.lineJoin = "round"
            ctx.lineCap = "round"
            ctx.beginPath()

            var lastX = 0
            var lastY = 0
            var isFirstPoint = true

            for (var g = 0; g < count; g++) {
                var gValue = root.values[start + g]
                var gClamped = Math.max(minY, Math.min(maxY, gValue))
                var gNorm = (gClamped - minY) / rangeY

                var gx = chartLeft + (g * stepX)
                var gy = chartBottom - (gNorm * chartHeight)

                if (isFirstPoint) {
                    ctx.moveTo(gx, gy)
                    isFirstPoint = false
                } else {
                    ctx.lineTo(gx, gy)
                }

                lastX = gx
                lastY = gy
            }
            ctx.stroke()

            // Main signal line
            ctx.strokeStyle = root.effectiveLineColor
            ctx.lineWidth = 2
            ctx.lineJoin = "round"
            ctx.lineCap = "round"
            ctx.beginPath()

            isFirstPoint = true
            for (var i = 0; i < count; i++) {
                var value = root.values[start + i]
                var clampedValue = Math.max(minY, Math.min(maxY, value))
                var normalized = (clampedValue - minY) / rangeY

                var x = chartLeft + (i * stepX)
                var y = chartBottom - (normalized * chartHeight)

                if (isFirstPoint) {
                    ctx.moveTo(x, y)
                    isFirstPoint = false
                } else {
                    ctx.lineTo(x, y)
                }

                lastX = x
                lastY = y
            }
            ctx.stroke()

            // ============================
            // 8. LIVE VALUE + DOT
            // ============================
            if (count > 0) {
                // Outer glow
                ctx.fillStyle = Qt.rgba(
                    root.effectiveLineColor.r,
                    root.effectiveLineColor.g,
                    root.effectiveLineColor.b,
                    0.25
                )
                ctx.beginPath()
                ctx.arc(lastX, lastY, 8, 0, 2 * Math.PI)
                ctx.fill()

                // Inner dot
                ctx.fillStyle = root.effectiveLineColor
                ctx.beginPath()
                ctx.arc(lastX, lastY, 4, 0, 2 * Math.PI)
                ctx.fill()

                // Center uses chart background so the marker stays readable in both themes.
                ctx.fillStyle = root.chartBackgroundColor
                ctx.beginPath()
                ctx.arc(lastX, lastY, 1.5, 0, 2 * Math.PI)
                ctx.fill()

                // Live value label above dot
                var liveValue = root.values[start + count - 1]
                ctx.fillStyle = root.effectiveLineColor
                ctx.font = "bold 11px sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "bottom"
                ctx.fillText(liveValue.toFixed(2) + " " + root.unit, lastX, lastY - 12)
            }
        }
    }

    // ===== Auto-calculate max from data =====
    function updateDynamicMax() {
        if (!root.values || root.values.length === 0) {
            root.calculatedMaxY = 1.0
            return
        }

        var max = root.values[0]

        for (var i = 0; i < root.values.length; i++) {
            if (root.values[i] > max) max = root.values[i]
        }

        // Round up to nice number with 20% padding
        var padded = max * 1.2

        // Round to nearest nice value
        if (padded <= 1) padded = 1
        else if (padded <= 2) padded = 2
        else if (padded <= 5) padded = 5
        else if (padded <= 10) padded = 10
        else if (padded <= 20) padded = 20
        else if (padded <= 50) padded = 50
        else if (padded <= 100) padded = 100
        else if (padded <= 200) padded = 200
        else padded = Math.ceil(padded / 50) * 50

        root.calculatedMaxY = padded
    }

    onValuesChanged: {
        if (root.values.length < root.lastValueCount) {
            root.discardedSamples += (root.lastValueCount - root.values.length)
        }
        root.lastValueCount = root.values.length

        updateDynamicMax()
        canvas.requestPaint()
    }

    onEffectiveLineColorChanged: canvas.requestPaint()
    onChartBackgroundColorChanged: canvas.requestPaint()
    onChartGridColorChanged: canvas.requestPaint()
    onAxisTextColorChanged: canvas.requestPaint()
    onChartBorderColorChanged: canvas.requestPaint()
}
