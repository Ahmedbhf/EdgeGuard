import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "components"
import "utils/HistoryUtils.js" as HistoryUtils
import "utils/ChartUtils.js" as ChartUtils
import EdgeGuard

Rectangle {
    id: root
    color: Theme.bg

    signal backClicked()

    property bool dataLoaded: false
    property string statusText: "Loading the last 24 hours of stored history..."
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real minWindowMs: 1000
    property real anomalyMinY: 0
    property real anomalyMaxY: 100
    property real rmsMinY: 0
    property real rmsMaxY: 1
    property real tempMinY: 0
    property real tempMaxY: 1
    property var anomalyPoints: []
    property var rmsPoints: []
    property var tempPoints: []
    function refreshHistory() {
        var csvText = dataModel.loadLast24hCsv()
        var parsed = HistoryUtils.parseCsv(csvText)
        if (!parsed.ok) {
            statusText = parsed.error
            dataLoaded = false
            anomalyPoints = []
            rmsPoints = []
            tempPoints = []
            return
        }

        fullStartMs = parsed.fullStartMs
        fullEndMs = parsed.fullEndMs
        minWindowMs = parsed.minWindowMs

        anomalyMinY = 0
        anomalyMaxY = 100

        var rmsRange = ChartUtils.computeRange(parsed.rmsValues)
        rmsMinY = rmsRange.min
        rmsMaxY = rmsRange.max

        var tempRange = ChartUtils.computeRange(parsed.tempValues)
        tempMinY = tempRange.min
        tempMaxY = tempRange.max

        anomalyPoints = parsed.anomalyPoints
        rmsPoints = parsed.rmsPoints
        tempPoints = parsed.tempPoints
        dataLoaded = true
        statusText = parsed.sampleCount + " samples loaded from the rolling 24-hour store. Hover to inspect points, drag horizontally to scroll, and use the mouse wheel to zoom."
    }

    function exportHistory() {
        exportDialog.open()
    }

    Component.onCompleted: Qt.callLater(refreshHistory)

    FileDialog {
        id: exportDialog
        title: "Export 24h History CSV"
        nameFilters: ["CSV files (*.csv)"]
        fileMode: FileDialog.SaveFile

        onAccepted: {
            if (dataModel.exportHistoryCsv(selectedFile))
                root.statusText = "Exported the current 24-hour history."
            else
                root.statusText = "Could not export the current 24-hour history."
        }
        onRejected: {
            if (root.dataLoaded)
                return
            root.statusText = "Export cancelled."
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
                text: "Refresh"
                primary: true
                onClicked: root.refreshHistory()
            }

            ControlButton {
                text: "Export CSV"
                onClicked: root.exportHistory()
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
            anomalyPoints: root.anomalyPoints
            anomalyMinY: root.anomalyMinY
            anomalyMaxY: root.anomalyMaxY
            rmsPoints: root.rmsPoints
            tempPoints: root.tempPoints
            rmsMinY: root.rmsMinY
            rmsMaxY: root.rmsMaxY
            tempMinY: root.tempMinY
            tempMaxY: root.tempMaxY
            fullStartMs: root.fullStartMs
            fullEndMs: root.fullEndMs
            minimumWindowMs: root.minWindowMs
            interactiveEnabled: root.dataLoaded
        }
    }
}
