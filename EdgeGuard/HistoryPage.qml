import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "components"
import EdgeGuard

Rectangle {
    id: root
    color: Theme.bg

    signal backClicked()

    property bool dataLoaded: false
    property string statusText: "Select a CSV file to view history."
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real viewStartMs: 0
    property real viewEndMs: 1000
    property real minWindowMs: 1000
    property real rmsMinY: 0
    property real rmsMaxY: 1
    property real tempMinY: 0
    property real tempMaxY: 1
    property var rmsPoints: []
    property var tempPoints: []

    function openHistoryFile() {
        fileDialog.open()
    }

    function parseTimestamp(text, sampleIndex) {
        var trimmed = text.trim()
        var direct = Date.parse(trimmed)
        if (!isNaN(direct))
            return direct

        var parts = trimmed.split(":")
        if (parts.length < 2)
            return sampleIndex * 1000

        var h = parseInt(parts[0], 10)
        var m = parseInt(parts[1], 10)
        var s = parts.length > 2 ? parseFloat(parts[2]) : 0
        if (isNaN(h) || isNaN(m) || isNaN(s))
            return sampleIndex * 1000

        var base = new Date()
        base.setHours(0, 0, 0, 0)
        return base.getTime() + (((h * 60) + m) * 60 + s) * 1000
    }

    function computeRange(values) {
        if (!values || values.length === 0)
            return { min: 0, max: 1 }

        var minValue = values[0]
        var maxValue = values[0]
        for (var i = 1; i < values.length; ++i) {
            minValue = Math.min(minValue, values[i])
            maxValue = Math.max(maxValue, values[i])
        }

        if (minValue === maxValue) {
            var pad = Math.max(1, Math.abs(minValue) * 0.2)
            return { min: minValue - pad, max: maxValue + pad }
        }

        var spread = maxValue - minValue
        return {
            min: Math.max(0, minValue - spread * 0.12),
            max: maxValue + spread * 0.12
        }
    }

    function setViewRange(startMs, endMs) {
        var span = Math.max(minWindowMs, endMs - startMs)
        var boundedStart = startMs
        var boundedEnd = endMs

        if (span >= (fullEndMs - fullStartMs)) {
            boundedStart = fullStartMs
            boundedEnd = fullEndMs
        } else {
            if (boundedStart < fullStartMs) {
                boundedStart = fullStartMs
                boundedEnd = boundedStart + span
            }
            if (boundedEnd > fullEndMs) {
                boundedEnd = fullEndMs
                boundedStart = boundedEnd - span
            }
        }

        viewStartMs = boundedStart
        viewEndMs = boundedEnd
    }

    function zoom(factor) {
        if (!dataLoaded)
            return

        var center = (viewStartMs + viewEndMs) / 2
        var span = Math.max(minWindowMs, (viewEndMs - viewStartMs) * factor)
        setViewRange(center - span / 2, center + span / 2)
    }

    function pan(pixelDelta, chartWidth) {
        if (!dataLoaded)
            return

        var span = viewEndMs - viewStartMs
        var shift = -(pixelDelta / Math.max(1, chartWidth)) * span
        setViewRange(viewStartMs + shift, viewEndMs + shift)
    }

    function loadCsv(fileUrl) {
        var csvText = dataModel.readTextFile(fileUrl)
        if (!csvText || csvText.length === 0) {
            statusText = "Could not read the selected CSV file."
            dataLoaded = false
            rmsPoints = []
            tempPoints = []
            return
        }

        var lines = csvText.split(/\r?\n/)
        if (lines.length < 2) {
            statusText = "CSV file is empty."
            dataLoaded = false
            rmsPoints = []
            tempPoints = []
            return
        }

        var headers = lines[0].split(",")
        var timeIndex = headers.indexOf("time")
        var rmsIndex = headers.indexOf("rms")
        var tempIndex = headers.indexOf("temp")
        if (timeIndex < 0 || rmsIndex < 0 || tempIndex < 0) {
            statusText = "CSV must contain time, rms, and temp columns."
            dataLoaded = false
            rmsPoints = []
            tempPoints = []
            return
        }

        var rmsValues = []
        var tempValues = []
        var nextRmsPoints = []
        var nextTempPoints = []
        var firstMs = -1
        var previousMs = -1
        var dayOffsetMs = 0

        for (var i = 1; i < lines.length; ++i) {
            var line = lines[i].trim()
            if (!line)
                continue

            var fields = line.split(",")
            if (fields.length <= Math.max(timeIndex, rmsIndex, tempIndex))
                continue

            var rmsValue = parseFloat(fields[rmsIndex].trim())
            var tempValue = parseFloat(fields[tempIndex].trim())
            if (isNaN(rmsValue) || isNaN(tempValue))
                continue

            var rawMs = parseTimestamp(fields[timeIndex], rmsValues.length)
            if (previousMs >= 0 && rawMs + dayOffsetMs < previousMs)
                dayOffsetMs += 24 * 60 * 60 * 1000

            var pointMs = rawMs + dayOffsetMs
            if (firstMs < 0)
                firstMs = pointMs
            previousMs = pointMs

            nextRmsPoints.push({ x: pointMs, y: rmsValue })
            nextTempPoints.push({ x: pointMs, y: tempValue })
            rmsValues.push(rmsValue)
            tempValues.push(tempValue)
        }

        if (rmsValues.length === 0 || tempValues.length === 0) {
            statusText = "No valid samples found in the CSV file."
            dataLoaded = false
            rmsPoints = []
            tempPoints = []
            return
        }

        var lastMs = previousMs > firstMs ? previousMs : firstMs + 1000
        fullStartMs = firstMs
        fullEndMs = lastMs
        viewStartMs = firstMs
        viewEndMs = lastMs
        minWindowMs = Math.max(1000, (fullEndMs - fullStartMs) / Math.min(20, rmsValues.length))
        if (fullEndMs === fullStartMs) {
            fullEndMs += 1000
            viewEndMs = fullEndMs
        }

        var rmsRange = computeRange(rmsValues)
        rmsMinY = rmsRange.min
        rmsMaxY = rmsRange.max

        var tempRange = computeRange(tempValues)
        tempMinY = tempRange.min
        tempMaxY = tempRange.max

        rmsPoints = nextRmsPoints
        tempPoints = nextTempPoints
        dataLoaded = true
        statusText = rmsValues.length + " samples loaded. Hover to inspect points, drag horizontally to scroll, and use the mouse wheel to zoom."
    }

    Component.onCompleted: Qt.callLater(openHistoryFile)

    FileDialog {
        id: fileDialog
        title: "Open History CSV"
        nameFilters: ["CSV files (*.csv)"]
        fileMode: FileDialog.OpenFile

        onAccepted: root.loadCsv(selectedFile)
        onRejected: {
            if (!root.dataLoaded)
                root.statusText = "No history CSV selected."
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ControlButton {
                text: "Back"
                onClicked: root.backClicked()
            }

            ControlButton {
                text: "Import CSV"
                primary: true
                onClicked: root.openHistoryFile()
            }

            Label {
                Layout.fillWidth: true
                text: root.statusText
                color: Theme.muted
                wrapMode: Text.WordWrap
                font.pixelSize: 12
            }
        }

        HistoryChart {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 2
            rmsPoints: root.rmsPoints
            tempPoints: root.tempPoints
            rmsMinY: root.rmsMinY
            rmsMaxY: root.rmsMaxY
            tempMinY: root.tempMinY
            tempMaxY: root.tempMaxY
            viewStartMs: root.viewStartMs
            viewEndMs: root.viewEndMs
            interactiveEnabled: root.dataLoaded
            onPanRequested: function(pixelDelta, chartWidth) { root.pan(pixelDelta, chartWidth) }
            onZoomRequested: function(factor) { root.zoom(factor) }
        }
    }
}
