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

    property var values: []
    property string unit: ""
    property int displayPoints: 60
    property color lineColor: Theme.primary
    property int topPadding: 24
    property int bottomPadding: 24

    // Dynamic min/max based on data
    property real calculatedMinY: 0.0
    property real calculatedMaxY: 1.0

    // Track total samples ever received
    property int discardedSamples: 0
    property int lastValueCount: 0

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Immediate

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var chartWidth = width
            var chartHeight = height - root.topPadding - root.bottomPadding

            // Draw faint horizontal center line
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(0, height / 2)
            ctx.lineTo(width, height / 2)
            ctx.stroke()

            if (!root.values || root.values.length === 0)
                return

            // Use calculated min/max from data
            var rangeY = root.calculatedMaxY - root.calculatedMinY
            if (rangeY === 0) rangeY = 1

            // Only show the last displayPoints
            var start = Math.max(0, root.values.length - root.displayPoints)
            var count = Math.min(root.values.length, root.displayPoints)

            // Spacing between points
            var stepX = chartWidth / Math.max(1, (root.displayPoints - 1))

            // Draw the Signal Line
            ctx.strokeStyle = root.lineColor
            ctx.lineWidth = 2
            ctx.lineJoin = "round"
            ctx.lineCap = "round"
            ctx.beginPath()

            var lastX = 0
            var lastY = 0
            var isFirstPoint = true

            for (var i = 0; i < count; i++) {
                var value = root.values[start + i]
                var clampedValue = Math.max(root.calculatedMinY, Math.min(root.calculatedMaxY, value))
                var normalized = (clampedValue - root.calculatedMinY) / rangeY

                var x = (i * stepX)
                var y = root.topPadding + chartHeight - (normalized * chartHeight)

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

            // Draw the live moving dot at the end
            if (count > 0) {
                ctx.fillStyle = root.lineColor
                ctx.beginPath()
                ctx.arc(lastX, lastY, 4, 0, 2 * Math.PI)
                ctx.fill()
            }
        }
    }

    // Dynamic min/max labels
    Text {
        text: root.calculatedMaxY.toFixed(2)
        color: Theme.muted
        font.pixelSize: 10
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
    }

    Text {
        text: root.calculatedMinY.toFixed(2)
        color: Theme.muted
        font.pixelSize: 10
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 8
    }

    Text {
        text: root.unit
        color: Theme.muted
        font.pixelSize: 10
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 8
    }

    // Calculate min/max from data with padding
    function updateMinMax() {
        if (!root.values || root.values.length === 0) {
            root.calculatedMinY = 0
            root.calculatedMaxY = 1
            return
        }

        var min = root.values[0]
        var max = root.values[0]

        // Find actual min/max in data
        for (var i = 0; i < root.values.length; i++) {
            if (root.values[i] < min) min = root.values[i]
            if (root.values[i] > max) max = root.values[i]
        }

        // Add 15% padding above and below
        var padding = (max - min) * 0.15
        if (padding === 0) padding = 1 // Prevent zero range

        root.calculatedMinY = min - padding
        root.calculatedMaxY = max + padding

        canvas.requestPaint()
    }

    onValuesChanged: {
        // Track discarded samples
        if (root.values.length < root.lastValueCount) {
            // Values were removed (buffer overflow)
            root.discardedSamples += (root.lastValueCount - root.values.length)
        }
        root.lastValueCount = root.values.length

        updateMinMax()
    }
}
