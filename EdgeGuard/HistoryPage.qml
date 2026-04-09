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

    property string selectedFftAxis: "X"
    readonly property var historyData: appController.historyData
    readonly property bool dataLoaded: historyData.sampleCount !== undefined && historyData.sampleCount > 0
    readonly property bool hasOlderHistory: historyData.hasOlder === true
    readonly property bool hasNewerHistory: historyData.hasNewer === true
    readonly property var fftAxisOptions: ["X", "Y", "Z"]
    readonly property var selectedFftMagnitudes: selectedFftAxis === "X"
                                               ? (historyData.fftXMagnitudes ? historyData.fftXMagnitudes : [])
                                               : (selectedFftAxis === "Y"
                                                      ? (historyData.fftYMagnitudes ? historyData.fftYMagnitudes : [])
                                                      : (historyData.fftZMagnitudes ? historyData.fftZMagnitudes : []))
    readonly property real selectedFftDominantFrequency: selectedFftAxis === "X"
                                                       ? (historyData.fftXDominantFrequency !== undefined ? historyData.fftXDominantFrequency : 0)
                                                       : (selectedFftAxis === "Y"
                                                              ? (historyData.fftYDominantFrequency !== undefined ? historyData.fftYDominantFrequency : 0)
                                                              : (historyData.fftZDominantFrequency !== undefined ? historyData.fftZDominantFrequency : 0))
    readonly property real selectedFftEnergy: selectedFftAxis === "X"
                                            ? (historyData.fftXEnergy !== undefined ? historyData.fftXEnergy : 0)
                                            : (selectedFftAxis === "Y"
                                                   ? (historyData.fftYEnergy !== undefined ? historyData.fftYEnergy : 0)
                                                   : (historyData.fftZEnergy !== undefined ? historyData.fftZEnergy : 0))
    readonly property real fftMaxFrequency: historyData.fftMaxFrequency !== undefined ? historyData.fftMaxFrequency : 1
    readonly property color selectedFftColor: selectedFftAxis === "X"
                                            ? "#86BBFF"
                                            : (selectedFftAxis === "Y" ? "#34D399" : "#F59E0B")

    function peakAmplitude(values) {
        if (!values || values.length === 0)
            return 0

        var peak = values[0]
        for (var index = 1; index < values.length; ++index)
            peak = Math.max(peak, values[index])
        return peak
    }

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

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 280
            spacing: 16

            ChartCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                Layout.horizontalStretchFactor: 68
                title: "FFT Analysis"

                headerContent: RowLayout {
                    spacing: Theme.spaceSm

                    AxisSelectorCombo {
                        Layout.preferredWidth: 84
                        model: root.fftAxisOptions
                        currentIndex: root.fftAxisOptions.indexOf(root.selectedFftAxis)
                        onActivated: root.selectedFftAxis = root.fftAxisOptions[currentIndex]
                    }

                    Label {
                        text: "Peak Amplitude " + root.peakAmplitude(root.selectedFftMagnitudes).toFixed(1)
                        color: Theme.muted
                        font.pixelSize: 12
                    }
                }

                FftSpectrumChart {
                    anchors.fill: parent
                    anchors.margins: 16
                    frequencies: historyData.fftFrequencies ? historyData.fftFrequencies : []
                    magnitudes: root.selectedFftMagnitudes
                    dominantFrequency: root.selectedFftDominantFrequency
                    lineColor: root.selectedFftColor
                }
            }

            GaugeCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                Layout.horizontalStretchFactor: 32
                value: root.selectedFftDominantFrequency
                max: root.fftMaxFrequency
                label: root.selectedFftAxis + " Dominant Frequency (Hz)"
                zones: [
                    { from: 0, to: root.fftMaxFrequency * 0.33, color: "green" },
                    { from: root.fftMaxFrequency * 0.33, to: root.fftMaxFrequency * 0.66, color: "yellow" },
                    { from: root.fftMaxFrequency * 0.66, to: root.fftMaxFrequency, color: "red" }
                ]
            }
        }
    }
}
