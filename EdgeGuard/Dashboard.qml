pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import EdgeGuard

Item {
    id: root

    property bool showingHistoryPage: false
    property string selectedAxis: "X"
    readonly property int liveDisplayPoints: 120
    readonly property bool compactLayout: width < 1100 || height < 700
    readonly property int contentMargin: compactLayout ? Theme.spaceMd : Theme.spaceLg
    readonly property int compactGaugeHeight: height < 600 ? 210 : 240
    readonly property int compactChartHeight: height < 600 ? 220 : 250
    readonly property var axisOptions: ["X", "Y", "Z"]
    readonly property var selectedAxisValues: selectedAxis === "X"
                                            ? appController.xAxisValues
                                            : (selectedAxis === "Y" ? appController.yAxisValues : appController.zAxisValues)
    readonly property color selectedAxisColor: selectedAxis === "X"
                                             ? "#86BBFF"
                                             : (selectedAxis === "Y" ? "#34D399" : "#F59E0B")

    function returnToSetup() {
        var stack = StackView.view
        if (!stack)
            return

        if (stack.depth > 1)
            stack.pop(StackView.Immediate)
        else
            stack.replace("SetupPage.qml", StackView.Immediate)
    }

    ColumnLayout {
        visible: !root.showingHistoryPage
        anchors.fill: parent
        spacing: Theme.spaceLg

        DashboardHeaderBar {
            Layout.fillWidth: true
            onConnectionToggled: {
                if (appController.connected)
                    appController.disconnectAndReset()
                else
                    root.returnToSetup()
            }
            onHistoryClicked: root.showingHistoryPage = true
            onThemeToggleClicked: Theme.toggleMode()
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: root.compactLayout ? compactDashboardContent : desktopDashboardContent
        }
    }

    Component {
        id: desktopDashboardContent

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: root.contentMargin
            anchors.rightMargin: root.contentMargin
            anchors.bottomMargin: root.contentMargin
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

                    ChartCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        title: "Anomaly Score vs Time"

                        LiveTrendChart {
                            anchors.fill: parent
                            anchors.margins: 16
                            values: appController.anomalyValues
                            showUnitLabel: false
                            displayPoints: root.liveDisplayPoints
                            sampleRateHz: 20.0
                            fixedMinY: 0
                            fixedMaxY: 100
                            lineColor: Theme.primary
                            anomalyActive: appController.state === "ANOMALY"
                        }
                    }

                    ChartCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0
                        title: "Acceleration"

                        headerContent: AxisSelectorCombo {
                            Layout.preferredWidth: 84
                            model: root.axisOptions
                            currentIndex: root.axisOptions.indexOf(root.selectedAxis)
                            onActivated: root.selectedAxis = root.axisOptions[currentIndex]
                        }

                        LiveTrendChart {
                            anchors.fill: parent
                            anchors.margins: 16
                            values: root.selectedAxisValues
                            unit: "mg"
                            showUnitLabel: false
                            displayPoints: root.liveDisplayPoints
                            sampleRateHz: 20.0
                            lineColor: root.selectedAxisColor
                            clampMinYToZero: false
                            anomalyActive: appController.state === "ANOMALY"
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
                        value: appController.anomalyScore
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
                        value: appController.temp
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

    Component {
        id: compactDashboardContent

        ScrollView {
            clip: true

            Item {
                width: parent.width
                implicitHeight: compactColumn.implicitHeight + root.contentMargin * 2

                Column {
                    id: compactColumn
                    x: root.contentMargin
                    y: root.contentMargin
                    width: parent.width - root.contentMargin * 2
                    spacing: Theme.spaceMd

                    GaugeCard {
                        width: parent.width
                        height: root.compactGaugeHeight
                        value: appController.anomalyScore
                        label: "Anomaly Score"
                        zones: [
                            { from: 0, to: 40, color: "red" },
                            { from: 40, to: 80, color: "yellow" },
                            { from: 80, to: 100, color: "green" }
                        ]
                    }

                    GaugeCard {
                        width: parent.width
                        height: root.compactGaugeHeight
                        value: appController.temp
                        max: 100
                        label: "Machine Temperature"
                        zones: [
                            { from: 0, to: 45, color: "green" },
                            { from: 45, to: 70, color: "yellow" },
                            { from: 70, to: 100, color: "red" }
                        ]
                    }

                    ChartCard {
                        width: parent.width
                        height: root.compactChartHeight
                        title: "Anomaly Score vs Time"

                        LiveTrendChart {
                            anchors.fill: parent
                            anchors.margins: 16
                            values: appController.anomalyValues
                            showUnitLabel: false
                            displayPoints: root.liveDisplayPoints
                            sampleRateHz: 20.0
                            fixedMinY: 0
                            fixedMaxY: 100
                            lineColor: Theme.primary
                            anomalyActive: appController.state === "ANOMALY"
                        }
                    }

                    ChartCard {
                        width: parent.width
                        height: root.compactChartHeight
                        title: "Acceleration"

                        headerContent: AxisSelectorCombo {
                            Layout.preferredWidth: 84
                            model: root.axisOptions
                            currentIndex: root.axisOptions.indexOf(root.selectedAxis)
                            onActivated: root.selectedAxis = root.axisOptions[currentIndex]
                        }

                        LiveTrendChart {
                            anchors.fill: parent
                            anchors.margins: 16
                            values: root.selectedAxisValues
                            unit: "mg"
                            showUnitLabel: false
                            displayPoints: root.liveDisplayPoints
                            sampleRateHz: 20.0
                            lineColor: root.selectedAxisColor
                            clampMinYToZero: false
                            anomalyActive: appController.state === "ANOMALY"
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: historyPageLoader
        anchors.fill: parent
        active: root.showingHistoryPage
        sourceComponent: Component {
            HistoryPage {
                onBackClicked: root.showingHistoryPage = false
            }
        }
    }
}
