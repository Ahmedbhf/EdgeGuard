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

    readonly property var historyData: appController.historyData
    readonly property bool dataLoaded: historyData.sampleCount !== undefined && historyData.sampleCount > 0
    readonly property bool hasOlderHistory: historyData.hasOlder === true
    readonly property bool hasNewerHistory: historyData.hasNewer === true

    Component.onCompleted: Qt.callLater(appController.refreshHistoryData)

    FileDialog {
        id: exportDialog
        title: "Export 24h History CSV"
        nameFilters: ["CSV files (*.csv)"]
        fileMode: FileDialog.SaveFile

        onAccepted: appController.exportHistoryCsv(selectedFile)
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
                onClicked: appController.refreshHistoryData()
            }

            ControlButton {
                text: "Older"
                enabled: hasOlderHistory
                onClicked: appController.loadOlderHistoryChunk()
            }

            ControlButton {
                text: "Newer"
                enabled: hasNewerHistory
                onClicked: appController.loadNewerHistoryChunk()
            }

            ControlButton {
                text: "Export CSV"
                onClicked: exportDialog.open()
            }

            Label {
                Layout.fillWidth: true
                text: appController.historyStatusText
                color: Theme.muted
                wrapMode: Text.WordWrap
                font.pixelSize: 12
            }
        }

        HistoryChart {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 420
            Layout.verticalStretchFactor: 1
            anomalyPoints: historyData.anomalyPoints ? historyData.anomalyPoints : []
            anomalyMinY: historyData.anomalyMinY !== undefined ? historyData.anomalyMinY : 0
            anomalyMaxY: historyData.anomalyMaxY !== undefined ? historyData.anomalyMaxY : 100
            tempPoints: historyData.tempPoints ? historyData.tempPoints : []
            tempMinY: historyData.tempMinY !== undefined ? historyData.tempMinY : 0
            tempMaxY: historyData.tempMaxY !== undefined ? historyData.tempMaxY : 1
            accelXPoints: historyData.accelXPoints ? historyData.accelXPoints : []
            accelYPoints: historyData.accelYPoints ? historyData.accelYPoints : []
            accelZPoints: historyData.accelZPoints ? historyData.accelZPoints : []
            accelXMinY: historyData.accelXMinY !== undefined ? historyData.accelXMinY : -1
            accelXMaxY: historyData.accelXMaxY !== undefined ? historyData.accelXMaxY : 1
            accelYMinY: historyData.accelYMinY !== undefined ? historyData.accelYMinY : -1
            accelYMaxY: historyData.accelYMaxY !== undefined ? historyData.accelYMaxY : 1
            accelZMinY: historyData.accelZMinY !== undefined ? historyData.accelZMinY : -1
            accelZMaxY: historyData.accelZMaxY !== undefined ? historyData.accelZMaxY : 1
            fullStartMs: historyData.fullStartMs !== undefined ? historyData.fullStartMs : 0
            fullEndMs: historyData.fullEndMs !== undefined ? historyData.fullEndMs : 1000
            minimumWindowMs: historyData.minimumWindowMs !== undefined ? historyData.minimumWindowMs : 1000
            interactiveEnabled: root.dataLoaded
        }
    }
}
