import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root

    property real value: 0
    property real min: 0
    property real max: 100
    property string label: ""
    property var zones: []

    readonly property real safeRange: Math.max(0.0001, max - min)
    readonly property real clampedValue: Math.max(min, Math.min(max, value))
    readonly property real progress: (clampedValue - min) / safeRange
    readonly property color gaugeColor: zoneColorForValue(clampedValue)
    readonly property color trackColor: Theme.lightMode ? "#d9e1ea" : "#1c2128"
    readonly property string displayValue: formatValue(clampedValue)

    radius: Theme.radiusLg
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1
    implicitHeight: 280

    function normalizeColor(value) {
        if (typeof value === "string") {
            if (value === "green" || value === "ok")
                return "#22c55e"
            if (value === "yellow" || value === "warning")
                return "#f59e0b"
            if (value === "red" || value === "fault")
                return "#ef4444"
        }

        return value || Theme.primary
    }

    function colorWithAlpha(value, alpha) {
        var color = normalizeColor(value)
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    function zoneColorForValue(position) {
        if (!zones || zones.length === 0)
            return Theme.primary

        for (var i = 0; i < zones.length; ++i) {
            var zone = zones[i]
            var isLastZone = i === zones.length - 1
            if (position >= zone.from && (position < zone.to || (isLastZone && position <= zone.to)))
                return normalizeColor(zone.color)
        }

        return normalizeColor(zones[zones.length - 1].color)
    }

    function valueToAngle(position) {
        return Math.PI + ((position - min) / safeRange) * Math.PI
    }

    function formatValue(position) {
        var rounded = Math.round(position * 10) / 10
        if (Math.abs(rounded - Math.round(rounded)) < 0.05)
            return Math.round(rounded).toString()
        return rounded.toFixed(1)
    }

    Item {
        id: contentBlock
        anchors.centerIn: parent
        width: parent.width - Theme.spaceLg * 2
        height: gaugeCanvas.height + Theme.spaceMd + valueColumn.implicitHeight

        Canvas {
            id: gaugeCanvas
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            height: Math.min(root.height * 0.5, width * 0.58)
            antialiasing: true

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var baseStrokeWidth = 18
                var zoneStrokeWidth = 16
                var activeStrokeWidth = 18
                var centerX = width / 2
                var centerY = height - 28
                var radius = Math.max(0, Math.min(width * 0.36, height - 50))
                var needleAngle = root.valueToAngle(root.clampedValue)
                var needleLength = radius - 24
                var innerRadius = 14
                var needleTailLength = 8

                ctx.lineCap = "round"

                ctx.beginPath()
                ctx.strokeStyle = root.trackColor
                ctx.lineWidth = baseStrokeWidth
                ctx.arc(centerX, centerY, radius, Math.PI, Math.PI * 2, false)
                ctx.stroke()

                if (root.zones && root.zones.length > 0) {
                    for (var i = 0; i < root.zones.length; ++i) {
                        var zone = root.zones[i]
                        var startAngle = root.valueToAngle(Math.max(root.min, zone.from))
                        var endAngle = root.valueToAngle(Math.min(root.max, zone.to))
                        var gap = 0.02

                        if (endAngle - startAngle <= gap * 2)
                            gap = 0

                        startAngle += gap
                        endAngle -= gap

                        ctx.beginPath()
                        ctx.strokeStyle = root.colorWithAlpha(zone.color, Theme.lightMode ? 0.95 : 0.9)
                        ctx.lineWidth = zoneStrokeWidth
                        ctx.arc(centerX, centerY, radius, startAngle, endAngle, false)
                        ctx.stroke()
                    }
                }

                ctx.beginPath()
                ctx.strokeStyle = root.gaugeColor
                ctx.lineWidth = activeStrokeWidth
                ctx.arc(centerX, centerY, radius, Math.PI, Math.PI + (root.progress * Math.PI), false)
                ctx.stroke()

                ctx.beginPath()
                ctx.strokeStyle = Theme.lightMode ? "#111827" : "#f8fafc"
                ctx.lineWidth = 5
                ctx.moveTo(
                    centerX - Math.cos(needleAngle) * needleTailLength,
                    centerY - Math.sin(needleAngle) * needleTailLength
                )
                ctx.lineTo(
                    centerX + Math.cos(needleAngle) * needleLength,
                    centerY + Math.sin(needleAngle) * needleLength
                )
                ctx.stroke()

                ctx.beginPath()
                ctx.fillStyle = root.gaugeColor
                ctx.arc(centerX, centerY, innerRadius / 2, 0, Math.PI * 2, false)
                ctx.fill()

                ctx.beginPath()
                ctx.fillStyle = Theme.lightMode ? "#f8fafc" : "#111827"
                ctx.arc(centerX, centerY, 4, 0, Math.PI * 2, false)
                ctx.fill()
            }
        }

        Column {
            id: valueColumn
            anchors.top: gaugeCanvas.bottom
            anchors.topMargin: Theme.spaceMd
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            spacing: 2

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                text: root.label
                color: Theme.muted
                font.pixelSize: 14
                font.weight: Font.Medium
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                text: root.displayValue
                color: Theme.text
                font.pixelSize: 44
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    onValueChanged: gaugeCanvas.requestPaint()
    onMinChanged: gaugeCanvas.requestPaint()
    onMaxChanged: gaugeCanvas.requestPaint()
    onZonesChanged: gaugeCanvas.requestPaint()
    onWidthChanged: gaugeCanvas.requestPaint()
    onHeightChanged: gaugeCanvas.requestPaint()
    Component.onCompleted: gaugeCanvas.requestPaint()

    Connections {
        target: Theme

        function onLightModeChanged() {
            gaugeCanvas.requestPaint()
        }
    }
}
