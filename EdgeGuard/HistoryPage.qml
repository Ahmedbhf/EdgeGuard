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
    property string statusText: "Select a CSV file to view history."
    property real fullStartMs: 0
    property real fullEndMs: 1000
    property real minWindowMs: 1000
    property real rmsMinY: 0
    property real rmsMaxY: 1
    property real tempMinY: 0
    property real tempMaxY: 1
    property var rmsPoints: []
    property var tempPoints: []
    readonly property int maxCsvRows: 5000

    function openHistoryFile() {
        fileDialog.open()
    }

    function loadCsv(fileUrl) {
        var csvText = dataModel.readTextFileLimited(fileUrl, root.maxCsvRows + 1)
        var parsed = HistoryUtils.parseCsv(csvText)
        if (!parsed.ok) {
            statusText = parsed.error
            dataLoaded = false
            rmsPoints = []
            tempPoints = []
            return
        }

        fullStartMs = parsed.fullStartMs
        fullEndMs = parsed.fullEndMs
        minWindowMs = parsed.minWindowMs

        var rmsRange = ChartUtils.computeRange(parsed.rmsValues)
        rmsMinY = rmsRange.min
        rmsMaxY = rmsRange.max

        var tempRange = ChartUtils.computeRange(parsed.tempValues)
        tempMinY = tempRange.min
        tempMaxY = tempRange.max

        rmsPoints = parsed.rmsPoints
        tempPoints = parsed.tempPoints
        dataLoaded = true
        statusText = parsed.sampleCount + " samples loaded (showing up to " + root.maxCsvRows + "). Hover to inspect points, drag horizontally to scroll, and use the mouse wheel to zoom."
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
            fullStartMs: root.fullStartMs
            fullEndMs: root.fullEndMs
            minimumWindowMs: root.minWindowMs
            interactiveEnabled: root.dataLoaded
        }
    }
}
