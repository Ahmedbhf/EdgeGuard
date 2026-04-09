import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import "../utils/HistoryChartUtils.js" as HistoryChartUtils
import EdgeGuard

ChartCard {
    id: root

    property alias chartTitle: root.title
    property alias chartView: chart
    property alias xAxis: axisX
    property alias yAxis: axisY
    property alias lineSeriesRef: lineSeries
    property alias highlightSeriesRef: highlightSeries
    property alias hoverLineRef: hoverLine
    property alias hoverCardRef: hoverCard
    property alias hoverTimeRef: hoverTime
    property alias hoverValueRef: hoverValue
    property alias contentAreaRef: contentArea
    property color lineColor: Theme.primary
    property string valueLabel: ""
    property string valueFormat: "%.2f"
    property int valueDecimals: 2
    property var points: []
    property var thresholdBands: []
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real minimumWindowMs: 1000
    property real axisMinY: 0
    property real axisMaxY: 1
    property bool interactiveEnabled: false
    property string timeFormat: "HH:mm:ss"
    property int xTickCount: 6

    function clampToAxis(value) {
        return Math.max(axisMinY, Math.min(axisMaxY, value))
    }

    function plotYForValue(value) {
        var span = Math.max(0.0001, axisMaxY - axisMinY)
        var normalized = (clampToAxis(value) - axisMinY) / span
        return chart.plotArea.y + chart.plotArea.height * (1 - normalized)
    }

    function applyVisibleRange(startMs, endMs) {
        return HistoryChartUtils.setVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, startMs, endMs)
    }

    function constrainCurrentRange() {
        return HistoryChartUtils.constrainVisibleRange(axisX,
                                                       axisY,
                                                       points,
                                                       axisMinY,
                                                       axisMaxY,
                                                       fullStartMs,
                                                       fullEndMs,
                                                       minimumWindowMs)
    }

    signal visibleRangeChanged(real startMs, real endMs)
    onPointsChanged: HistoryChartUtils.refreshSeries(lineSeries,
                                                     points,
                                                     highlightSeries,
                                                     hoverLine,
                                                     hoverCard,
                                                     axisX,
                                                     axisY,
                                                     axisMinY,
                                                     axisMaxY,
                                                     fullStartMs,
                                                     fullEndMs)
    onFullStartMsChanged: HistoryChartUtils.setVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, fullStartMs, fullEndMs)
    onFullEndMsChanged: HistoryChartUtils.setVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, fullStartMs, fullEndMs)
    onMinimumWindowMsChanged: HistoryChartUtils.constrainVisibleRange(axisX,
                                                                      axisY,
                                                                      points,
                                                                      axisMinY,
                                                                      axisMaxY,
                                                                      fullStartMs,
                                                                      fullEndMs,
                                                                      minimumWindowMs)
    onAxisMinYChanged: axisY.min = axisMinY
    onAxisMaxYChanged: axisY.max = axisMaxY

    Item {
        id: contentArea
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: Theme.panel2
            radius: 12
        }

        Item {
            id: thresholdBandLayer
            anchors.fill: parent
            clip: true
            visible: root.thresholdBands && root.thresholdBands.length > 0

            Repeater {
                model: root.thresholdBands

                Rectangle {
                    required property var modelData

                    readonly property real clippedFrom: root.clampToAxis(Math.min(modelData.from, modelData.to))
                    readonly property real clippedTo: root.clampToAxis(Math.max(modelData.from, modelData.to))
                    readonly property real topY: root.plotYForValue(clippedTo)
                    readonly property real bottomY: root.plotYForValue(clippedFrom)

                    visible: chart.plotArea.width > 0
                             && chart.plotArea.height > 0
                             && clippedTo > clippedFrom
                    x: chart.x + chart.plotArea.x
                    y: chart.y + topY
                    width: chart.plotArea.width
                    height: Math.max(0, bottomY - topY)
                    color: modelData.color
                    opacity: modelData.opacity !== undefined ? modelData.opacity : 0.16
                }
            }
        }

        ChartView {
            id: chart
            anchors.fill: parent
            anchors.margins: 16
            antialiasing: true
            legend.visible: false
            backgroundColor: "transparent"
            plotAreaColor: "transparent"
            margins.left: 8
            margins.right: 8
            margins.top: 8
            margins.bottom: 8

            DateTimeAxis {
                id: axisX
                min: new Date(root.fullStartMs)
                max: new Date(root.fullEndMs)
                format: root.timeFormat
                tickCount: root.xTickCount
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
                        // Built-in chart scrolling keeps the drag behavior responsive without custom time math.
                        var delta = mouse.x - dragStartX
                        if (delta > 0)
                            chart.scrollLeft(delta)
                        else if (delta < 0)
                            chart.scrollRight(-delta)
                        var constrainedRange = HistoryChartUtils.constrainVisibleRange(axisX,
                                                                                       axisY,
                                                                                       points,
                                                                                       axisMinY,
                                                                                       axisMaxY,
                                                                                       fullStartMs,
                                                                                       fullEndMs,
                                                                                       minimumWindowMs)
                        if (constrainedRange)
                            root.visibleRangeChanged(constrainedRange.startMs, constrainedRange.endMs)
                        dragStartX = mouse.x
                    } else {
                        HistoryChartUtils.updateHover(chart,
                                                      lineSeries,
                                                      points,
                                                      interactiveEnabled,
                                                      highlightSeries,
                                                      hoverLine,
                                                      hoverCard,
                                                      hoverTime,
                                                      hoverValue,
                                                      valueLabel,
                                                      valueDecimals,
                                                      contentArea.width,
                                                      contentArea.height,
                                                      mouse.x,
                                                      mouse.y)
                    }
                }

                onReleased: function(mouse) {
                    HistoryChartUtils.updateHover(chart,
                                                  lineSeries,
                                                  points,
                                                  interactiveEnabled,
                                                  highlightSeries,
                                                  hoverLine,
                                                  hoverCard,
                                                  hoverTime,
                                                  hoverValue,
                                                  valueLabel,
                                                  valueDecimals,
                                                  contentArea.width,
                                                  contentArea.height,
                                                  mouse.x,
                                                  mouse.y)
                }

                onExited: HistoryChartUtils.resetHover(highlightSeries, hoverLine, hoverCard)

                onWheel: function(wheel) {
                    // Use Qt Charts' built-in zoom methods, then clamp back to the loaded history extent.
                    if (wheel.angleDelta.y > 0)
                        chart.zoomIn()
                    else if (wheel.angleDelta.y < 0)
                        chart.zoomOut()
                    var constrainedRange = HistoryChartUtils.constrainVisibleRange(axisX,
                                                                                   axisY,
                                                                                   points,
                                                                                   axisMinY,
                                                                                   axisMaxY,
                                                                                   fullStartMs,
                                                                                   fullEndMs,
                                                                                   minimumWindowMs)
                    if (constrainedRange)
                        root.visibleRangeChanged(constrainedRange.startMs, constrainedRange.endMs)
                    HistoryChartUtils.updateHover(chart,
                                                  lineSeries,
                                                  points,
                                                  interactiveEnabled,
                                                  highlightSeries,
                                                  hoverLine,
                                                  hoverCard,
                                                  hoverTime,
                                                  hoverValue,
                                                  valueLabel,
                                                  valueDecimals,
                                                  contentArea.width,
                                                  contentArea.height,
                                                  wheel.x,
                                                  wheel.y)
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
