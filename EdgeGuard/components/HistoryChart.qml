import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../utils/HistoryChartUtils.js" as HistoryChartUtils
import EdgeGuard

Item {
    id: root

    property var anomalyPoints: []
    property real anomalyMinY: 0
    property real anomalyMaxY: 100
    property var tempPoints: []
    property real tempMinY: 0
    property real tempMaxY: 1
    property var accelXPoints: []
    property var accelYPoints: []
    property var accelZPoints: []
    property real accelXMinY: -1
    property real accelXMaxY: 1
    property real accelYMinY: -1
    property real accelYMaxY: 1
    property real accelZMinY: -1
    property real accelZMaxY: 1
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real minimumWindowMs: 1000
    property bool interactiveEnabled: false
    property int selectedMetricIndex: 0
    property int selectedAccelerationAxisIndex: 0
    property int selectedWindowIndex: 1
    property real visibleStartMs: 0
    property real visibleEndMs: 0
    property bool viewportReady: false

    readonly property var windowOptions: [
        { label: "15 min", durationMs: 15 * 60 * 1000 },
        { label: "1 hour", durationMs: 60 * 60 * 1000 },
        { label: "6 hours", durationMs: 6 * 60 * 60 * 1000 },
        { label: "24 hours", durationMs: 24 * 60 * 60 * 1000 }
    ]
    readonly property var windowLabels: windowOptions.map(function(option) { return option.label })
    readonly property real fullSpanMs: Math.max(minimumWindowMs, fullEndMs - fullStartMs)
    readonly property real selectedWindowMs: Math.min(fullSpanMs, windowOptions[selectedWindowIndex].durationMs)
    readonly property real currentWindowSpanMs: Math.max(minimumWindowMs,
                                                         visibleEndMs > visibleStartMs ? (visibleEndMs - visibleStartMs)
                                                                                       : selectedWindowMs)
    readonly property real maxStartOffsetMs: Math.max(0, fullSpanMs - currentWindowSpanMs)
    readonly property real navigatorPosition: maxStartOffsetMs > 0
                                             ? Math.max(0, Math.min(1, (visibleStartMs - fullStartMs) / maxStartOffsetMs))
                                             : 1
    readonly property string currentTimeFormat: currentWindowSpanMs >= 6 * 60 * 60 * 1000 ? "HH:mm"
                                                                                           : "HH:mm:ss"
    readonly property int currentTickCount: currentWindowSpanMs >= 6 * 60 * 60 * 1000 ? 5 : 6
    readonly property string currentWindowLabel: windowOptions[selectedWindowIndex].label
    readonly property string visibleRangeLabel: viewportReady
                                                ? (Qt.formatDateTime(new Date(visibleStartMs), currentTimeFormat)
                                                   + " - "
                                                   + Qt.formatDateTime(new Date(visibleEndMs), currentTimeFormat))
                                                : ""

    readonly property bool showingAcceleration: selectedMetricIndex === 2
    readonly property string selectedAccelerationAxis: selectedAccelerationAxisIndex === 1
                                                       ? "Y"
                                                       : (selectedAccelerationAxisIndex === 2 ? "Z" : "X")
    readonly property string currentTitle: {
        if (showingAcceleration)
            return "Acceleration " + selectedAccelerationAxis + " vs Time"
        if (selectedMetricIndex === 1)
            return "Temperature vs Time"
            return "Similarity vs Time"
    }
    readonly property string currentValueLabel: {
        if (showingAcceleration)
            return "Accel " + selectedAccelerationAxis
        if (selectedMetricIndex === 1)
            return "Temp"
        return "Score"
    }
    readonly property string currentValueFormat: showingAcceleration ? "%.3f" : "%.1f"
    readonly property int currentValueDecimals: showingAcceleration ? 3 : 1
    readonly property color currentLineColor: {
        if (showingAcceleration) {
            if (selectedAccelerationAxisIndex === 1)
                return "#10B981"
            if (selectedAccelerationAxisIndex === 2)
                return "#F59E0B"
            return Theme.primary
        }

        return selectedMetricIndex === 1 ? "#F59E0B" : Theme.text
    }
    readonly property var currentThresholdBands: selectedMetricIndex === 0 ? [
        { from: 0, to: 50, color: "#EF4444", opacity: 0.16 },
        { from: 50, to: 80, color: "#F59E0B", opacity: 0.14 },
        { from: 80, to: 100, color: "#22C55E", opacity: 0.14 }
    ] : []
    readonly property var currentPoints: {
        if (showingAcceleration) {
            if (selectedAccelerationAxisIndex === 1)
                return accelYPoints
            if (selectedAccelerationAxisIndex === 2)
                return accelZPoints
            return accelXPoints
        }

        return selectedMetricIndex === 1 ? tempPoints : anomalyPoints
    }
    readonly property real currentMinY: {
        if (showingAcceleration) {
            if (selectedAccelerationAxisIndex === 1)
                return accelYMinY
            if (selectedAccelerationAxisIndex === 2)
                return accelZMinY
            return accelXMinY
        }

        return selectedMetricIndex === 1 ? tempMinY : anomalyMinY
    }
    readonly property real currentMaxY: {
        if (showingAcceleration) {
            if (selectedAccelerationAxisIndex === 1)
                return accelYMaxY
            if (selectedAccelerationAxisIndex === 2)
                return accelZMaxY
            return accelXMaxY
        }

        return selectedMetricIndex === 1 ? tempMaxY : anomalyMaxY
    }

    function applyViewport(startMs, endMs) {
        if (!HistoryChartUtils.hasPoints(currentPoints))
            return

        var normalized = HistoryChartUtils.normalizeVisibleRange(startMs,
                                                                 endMs,
                                                                 fullStartMs,
                                                                 fullEndMs,
                                                                 minimumWindowMs)
        visibleStartMs = normalized.startMs
        visibleEndMs = normalized.endMs
        viewportReady = true
        chartPanel.applyVisibleRange(visibleStartMs, visibleEndMs)
    }

    function showLatestWindow() {
        var span = Math.max(minimumWindowMs, Math.min(fullSpanMs, selectedWindowMs))
        applyViewport(fullEndMs - span, fullEndMs)
    }

    function shiftWindow(direction) {
        if (!viewportReady)
            showLatestWindow()

        var step = Math.max(minimumWindowMs, currentWindowSpanMs * 0.8)
        applyViewport(visibleStartMs + (direction * step), visibleEndMs + (direction * step))
    }

    function scrubTo(position) {
        if (maxStartOffsetMs <= 0) {
            showLatestWindow()
            return
        }

        var span = Math.max(minimumWindowMs, Math.min(fullSpanMs, currentWindowSpanMs))
        var startMs = fullStartMs + (Math.max(0, Math.min(1, position)) * maxStartOffsetMs)
        applyViewport(startMs, startMs + span)
    }

    function syncViewport() {
        if (!HistoryChartUtils.hasPoints(currentPoints))
            return

        if (!viewportReady) {
            showLatestWindow()
            return
        }

        applyViewport(visibleStartMs, visibleEndMs)
    }

    onCurrentPointsChanged: Qt.callLater(syncViewport)
    onCurrentMinYChanged: if (viewportReady) Qt.callLater(syncViewport)
    onCurrentMaxYChanged: if (viewportReady) Qt.callLater(syncViewport)
    onFullStartMsChanged: Qt.callLater(syncViewport)
    onFullEndMsChanged: Qt.callLater(syncViewport)
    onSelectedWindowIndexChanged: Qt.callLater(showLatestWindow)
    Component.onCompleted: Qt.callLater(syncViewport)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        HistoryChartPanel {
            id: chartPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            chartTitle: root.currentTitle
            valueLabel: root.currentValueLabel
            valueFormat: root.currentValueFormat
            valueDecimals: root.currentValueDecimals
            lineColor: root.currentLineColor
            thresholdBands: root.currentThresholdBands
            points: root.currentPoints
            axisMinY: root.currentMinY
            axisMaxY: root.currentMaxY
            fullStartMs: root.fullStartMs
            fullEndMs: root.fullEndMs
            minimumWindowMs: root.minimumWindowMs
            interactiveEnabled: root.interactiveEnabled
            timeFormat: root.currentTimeFormat
            xTickCount: root.currentTickCount
            onVisibleRangeChanged: function(startMs, endMs) {
                root.visibleStartMs = startMs
                root.visibleEndMs = endMs
                root.viewportReady = true
            }

            headerContent: RowLayout {
                spacing: Theme.spaceSm

                Label {
                    text: "Metric"
                    color: Theme.muted
                    font.pixelSize: 12
                }

                AxisSelectorCombo {
            model: ["Similarity", "Temperature", "Acceleration"]
                    currentIndex: root.selectedMetricIndex
                    implicitWidth: 156
                    onActivated: function(index) {
                        root.selectedMetricIndex = index
                    }
                }

                Label {
                    text: "Window"
                    color: Theme.muted
                    font.pixelSize: 12
                }

                AxisSelectorCombo {
                    model: root.windowLabels
                    currentIndex: root.selectedWindowIndex
                    implicitWidth: 104
                    onActivated: function(index) {
                        root.selectedWindowIndex = index
                    }
                }

                ControlButton {
                    text: "Prev"
                    implicitHeight: 34
                    enabled: root.viewportReady && root.visibleStartMs > root.fullStartMs
                    onClicked: root.shiftWindow(-1)
                }

                ControlButton {
                    text: "Next"
                    implicitHeight: 34
                    enabled: root.viewportReady && root.visibleEndMs < root.fullEndMs
                    onClicked: root.shiftWindow(1)
                }

                ControlButton {
                    text: "Latest"
                    implicitHeight: 34
                    primary: true
                    enabled: HistoryChartUtils.hasPoints(root.currentPoints)
                             && (!root.viewportReady || root.visibleEndMs < root.fullEndMs)
                    onClicked: root.showLatestWindow()
                }

                AxisSelectorCombo {
                    visible: root.showingAcceleration
                    enabled: visible
                    model: ["X Axis", "Y Axis", "Z Axis"]
                    currentIndex: root.selectedAccelerationAxisIndex
                    implicitWidth: 110
                    onActivated: function(index) {
                        root.selectedAccelerationAxisIndex = index
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            radius: Theme.radiusMd
            color: Theme.panel
            border.width: 1
            border.color: Theme.borderSoft
            visible: HistoryChartUtils.hasPoints(root.currentPoints)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceMd

                Label {
                    text: root.currentWindowLabel
                    color: Theme.text
                    font.pixelSize: 12
                    font.bold: true
                }

                Label {
                    text: root.visibleRangeLabel
                    color: Theme.muted
                    font.pixelSize: 12
                }

                Slider {
                    id: timelineSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: root.navigatorPosition
                    enabled: root.maxStartOffsetMs > 0

                    background: Rectangle {
                        x: timelineSlider.leftPadding
                        y: timelineSlider.topPadding + (timelineSlider.availableHeight - height) / 2
                        width: timelineSlider.availableWidth
                        height: 4
                        radius: 2
                        color: Theme.borderSoft

                        Rectangle {
                            width: timelineSlider.visualPosition * parent.width
                            height: parent.height
                            radius: parent.radius
                            color: Theme.primary
                        }
                    }

                    handle: Rectangle {
                        x: timelineSlider.leftPadding + (timelineSlider.visualPosition * (timelineSlider.availableWidth - width))
                        y: timelineSlider.topPadding + (timelineSlider.availableHeight - height) / 2
                        width: 14
                        height: 14
                        radius: 7
                        color: timelineSlider.pressed ? Theme.text : Theme.primary
                    }

                    onMoved: root.scrubTo(value)
                }

                Label {
                    text: Qt.formatDateTime(new Date(root.fullEndMs), root.currentTimeFormat)
                    color: Theme.muted
                    font.pixelSize: 12
                }
            }
        }
    }
}
