pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import EdgeGuard

Item {
    id: root
    anchors.fill: parent

    // The dashboard swaps to the history page without leaving the current stack entry.
    property bool showingHistoryPage: false
    property var anomalyHistory: []
    readonly property int liveDisplayPoints: 120

    function appendHistoryValue(series, nextValue, maxPoints) {
        var nextSeries = series ? series.slice(0) : []
        nextSeries.push(nextValue)

        if (nextSeries.length > maxPoints)
            nextSeries = nextSeries.slice(nextSeries.length - maxPoints)

        return nextSeries
    }

    function returnToSetup() {
        var stack = StackView.view
        if (!stack)
            return

        if (stack.depth > 1)
            stack.pop(StackView.Immediate)
        else
            stack.replace("SetupPage.qml", StackView.Immediate)
    }

    Connections {
        target: dataModel

        function onDataChanged() {
            // Keep a lightweight local score history for the dashboard without changing backend state.
            root.anomalyHistory = root.appendHistoryValue(root.anomalyHistory, dataModel.anomalyScore, 300)
        }
    }

    ColumnLayout {
        visible: !root.showingHistoryPage
        anchors.fill: parent
        spacing: Theme.spaceLg

        DashboardHeaderBar {
            Layout.fillWidth: true
            onConnectionToggled: {
                // Disconnecting also clears the detected identity so setup starts fresh next time.
                if (dataModel.connected) {
                    dataModel.disconnectPort()
                    dataModel.deviceId = ""
                    dataModel.machineType = ""
                } else {
                    root.returnToSetup()
                }
            }
            onExportCsvClicked: dataModel.openCsvFile()
            onHistoryClicked: root.showingHistoryPage = true
            onThemeToggleClicked: Theme.toggleMode()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16
            spacing: Theme.spaceLg

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                spacing: Theme.spaceLg

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 0
                    Layout.horizontalStretchFactor: 65
                    spacing: Theme.spaceLg

                    PanelCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        title: "Anomaly Score vs Time"

                        LiveTrendChart {
                            anchors.fill: parent
                            anchors.margins: 16
                            values: root.anomalyHistory
                            showUnitLabel: false
                            displayPoints: root.liveDisplayPoints
                            sampleRateHz: 20.0
                            fixedMinY: 0
                            fixedMaxY: 100
                            lineColor: Theme.primary
                            anomalyActive: dataModel.state === "ANOMALY"
                        }
                    }

                    PanelCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        title: "RMS Vibration vs Time"

                        LiveTrendChart {
                            anchors.fill: parent
                            anchors.margins: 16
                            values: dataModel.vibrationValues
                            unit: "mg"
                            showUnitLabel: false
                            displayPoints: root.liveDisplayPoints
                            sampleRateHz: 20.0
                            lineColor: "#86BBFF"
                            anomalyActive: dataModel.state === "ANOMALY"
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 0
                    Layout.horizontalStretchFactor: 35
                    spacing: Theme.spaceLg

                    GaugeCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        value: dataModel.anomalyScore
                        label: "Anomaly Score"
                        zones: [
                            { from: 0, to: 40, color: "red" },
                            { from: 40, to: 80, color: "yellow" },
                            { from: 80, to: 100, color: "green" }
                        ]
                    }

                    GaugeCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        value: dataModel.temp
                        max: 100
                        label: "Machine Temperature"
                        zones: [
                            { from: 0, to: 45, color: "green" },
                            { from: 45, to: 70, color: "yellow" },
                            { from: 70, to: 100, color: "red" }
                        ]
                    }
                }
            }
        }
    }

    Loader {
        id: historyPageLoader
        anchors.fill: parent
        // Load the history screen only when needed so the dashboard stays lightweight.
        active: root.showingHistoryPage
        sourceComponent: Component {
            HistoryPage {
                onBackClicked: root.showingHistoryPage = false
            }
        }
    }
}
