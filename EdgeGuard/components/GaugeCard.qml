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
    readonly property color trackColor: Theme.lightMode ? "#e8edf4" : "#1a1d23"
    readonly property string displayValue: formatValue(clampedValue)

    radius: Theme.radiusLg
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1
    implicitHeight: 280

    function normalizeColor(value) {
        if (typeof value === "string") {
            if (value === "green" || value === "ok")
                return Theme.ok
            if (value === "yellow" || value === "warning")
                return Theme.warning
            if (value === "red" || value === "fault")
                return Theme.fault
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

    Canvas {
        id: gaugeCanvas
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceLg
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        height: Math.min(parent.height * 0.62, width * 0.68)
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var strokeWidth = 14
            var centerX = width / 2
            var centerY = height - strokeWidth
            var radius = Math.max(0, Math.min(width * 0.38, height - strokeWidth * 1.5))

            ctx.lineCap = "round"

            ctx.beginPath()
            ctx.strokeStyle = root.trackColor
            ctx.lineWidth = strokeWidth
            ctx.arc(centerX, centerY, radius, Math.PI, Math.PI * 2, false)
            ctx.stroke()

            if (root.zones && root.zones.length > 0) {
                for (var i = 0; i < root.zones.length; ++i) {
                    var zone = root.zones[i]
                    var startAngle = root.valueToAngle(Math.max(root.min, zone.from))
                    var endAngle = root.valueToAngle(Math.min(root.max, zone.to))

                    ctx.beginPath()
                    ctx.strokeStyle = root.colorWithAlpha(zone.color, Theme.lightMode ? 0.18 : 0.26)
                    ctx.lineWidth = strokeWidth
                    ctx.arc(centerX, centerY, radius, startAngle, endAngle, false)
                    ctx.stroke()
                }
            }

            ctx.beginPath()
            ctx.strokeStyle = root.gaugeColor
            ctx.lineWidth = strokeWidth
            ctx.arc(centerX, centerY, radius, Math.PI, Math.PI + (root.progress * Math.PI), false)
            ctx.stroke()
        }
    }

    Column {
        anchors.horizontalCenter: gaugeCanvas.horizontalCenter
        anchors.verticalCenter: gaugeCanvas.verticalCenter
        anchors.verticalCenterOffset: gaugeCanvas.height * 0.18
        spacing: Theme.spaceXs

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.displayValue
            color: Theme.text
            font.pixelSize: 40
            font.weight: Font.DemiBold
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            color: Theme.muted
            font.pixelSize: 13
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
