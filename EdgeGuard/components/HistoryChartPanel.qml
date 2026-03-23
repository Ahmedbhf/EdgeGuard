import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import "../utils/HistoryChartUtils.js" as HistoryChartUtils
import EdgeGuard

PanelCard {
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
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real minimumWindowMs: 1000
    property real axisMinY: 0
    property real axisMaxY: 1
    property bool interactiveEnabled: false

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

        ChartView {
            id: chart
            anchors.fill: parent
            anchors.margins: 16
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
                min: new Date(root.fullStartMs)
                max: new Date(root.fullEndMs)
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
