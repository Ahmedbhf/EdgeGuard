import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import EdgeGuard

Rectangle {
    id: root
    radius: 16
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    property string chartTitle: ""
    property color lineColor: Theme.primary
    property string valueLabel: ""
    property string valueFormat: "%.2f"
    property int valueDecimals: 2
    property var points: []
    property real viewStartMs: 0
    property real viewEndMs: 1000
    property real axisMinY: 0
    property real axisMaxY: 1
    property bool interactiveEnabled: false

    signal panRequested(real pixelDelta, real chartWidth)
    signal zoomRequested(real factor)

    function resetHover() {
        highlightSeries.clear()
        hoverLine.visible = false
        hoverCard.visible = false
    }

    function hasPoints() {
        return points && points.length > 0
    }

    function populateSeries() {
        lineSeries.clear()
        if (!hasPoints())
            return

        for (var i = 0; i < points.length; ++i)
            lineSeries.append(points[i].x, points[i].y)
    }

    function nearestIndex(targetX) {
        if (!hasPoints())
            return -1

        var left = 0
        var right = points.length - 1
        while (left < right) {
            var mid = Math.floor((left + right) / 2)
            if (points[mid].x < targetX)
                left = mid + 1
            else
                right = mid
        }

        var candidate = left
        if (candidate > 0 && Math.abs(points[candidate - 1].x - targetX) < Math.abs(points[candidate].x - targetX))
            candidate -= 1
        return candidate
    }

    function inPlotArea(plot, mouseX, mouseY) {
        return mouseX >= plot.x && mouseX <= plot.x + plot.width
                && mouseY >= plot.y && mouseY <= plot.y + plot.height
    }

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function refreshSeries() {
        resetHover()
        populateSeries()
    }

    function updateHover(mouseX, mouseY) {
        if (!interactiveEnabled || !hasPoints()) {
            resetHover()
            return
        }

        var plot = chart.plotArea
        if (!inPlotArea(plot, mouseX, mouseY)) {
            resetHover()
            return
        }

        var mapped = chart.mapToValue(Qt.point(mouseX, mouseY), lineSeries)
        var index = nearestIndex(mapped.x)
        if (index < 0) {
            resetHover()
            return
        }

        var point = points[index]
        var scenePoint = chart.mapToPosition(Qt.point(point.x, point.y), lineSeries)

        highlightSeries.clear()
        highlightSeries.append(point.x, point.y)

        hoverLine.visible = true
        hoverLine.x = clamp(scenePoint.x - hoverLine.width / 2,
                            plot.x,
                            plot.x + plot.width - hoverLine.width)
        hoverLine.y = plot.y
        hoverLine.height = plot.height

        hoverCard.visible = true
        hoverTime.text = Qt.formatDateTime(new Date(point.x), "HH:mm:ss")
        hoverValue.text = valueLabel + ": " + point.y.toFixed(valueDecimals)

        var desiredX = scenePoint.x + 14
        var maxX = width - hoverCard.width - 12
        if (desiredX > maxX)
            desiredX = scenePoint.x - hoverCard.width - 14
        hoverCard.x = clamp(desiredX, 12, maxX)

        var desiredY = scenePoint.y - hoverCard.height - 12
        if (desiredY < 12)
            desiredY = scenePoint.y + 12
        hoverCard.y = clamp(desiredY, 12, height - hoverCard.height - 12)
    }

    onPointsChanged: refreshSeries()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: root.chartTitle
            color: Theme.text
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }

        ChartView {
            id: chart
            Layout.fillWidth: true
            Layout.fillHeight: true
            antialiasing: true
            legend.visible: false
            backgroundColor: Theme.panel2
            plotAreaColor: Theme.panel2
            margins.left: 8
            margins.right: 8
            margins.top: 8
            margins.bottom: 8

            DateTimeAxis {
                id: axisX
                min: new Date(root.viewStartMs)
                max: new Date(root.viewEndMs)
                format: "HH:mm:ss"
                tickCount: 6
                labelsColor: Theme.text
                gridLineColor: Theme.borderSoft
            }

            ValueAxis {
                id: axisY
                min: root.axisMinY
                max: root.axisMaxY
                labelsColor: Theme.text
                gridLineColor: Theme.borderSoft
                labelFormat: root.valueFormat
            }

            LineSeries {
                id: lineSeries
                axisX: axisX
                axisY: axisY
                color: root.lineColor
                width: 2
            }

            ScatterSeries {
                id: highlightSeries
                axisX: axisX
                axisY: axisY
                color: root.lineColor
                borderColor: "#ffffff"
                markerSize: 10
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                property real dragStartX: 0

                onPressed: function(mouse) {
                    dragStartX = mouse.x
                }

                onPositionChanged: function(mouse) {
                    if (mouse.buttons & Qt.LeftButton) {
                        root.panRequested(mouse.x - dragStartX, width)
                        dragStartX = mouse.x
                    } else {
                        root.updateHover(mouse.x, mouse.y)
                    }
                }

                onReleased: function(mouse) {
                    root.updateHover(mouse.x, mouse.y)
                }

                onExited: root.resetHover()

                onWheel: function(wheel) {
                    root.zoomRequested(wheel.angleDelta.y > 0 ? 0.8 : 1.25)
                    wheel.accepted = true
                }
            }
        }
    }

    Rectangle {
        id: hoverLine
        visible: false
        width: 1
        color: Theme.borderSoft
        opacity: 0.9
    }

    Rectangle {
        id: hoverCard
        visible: false
        width: 126
        height: 56
        radius: 12
        color: Theme.panel
        border.color: Theme.borderSoft
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 2

            Text {
                id: hoverTime
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
            }

            Text {
                id: hoverValue
                color: Theme.muted
                font.pixelSize: 11
            }
        }
    }
}
