pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import EdgeGuard

Item {
    id: root
    anchors.fill: parent

    property bool showingHistoryPage: false

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
                spacing: Theme.spaceLg
                Layout.minimumHeight: 0

                LiveChartsPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.horizontalStretchFactor: 7
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: 860
                }

                MetricsOverviewPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.horizontalStretchFactor: 5
                    Layout.minimumWidth: 420
                    Layout.preferredWidth: 520
                }

                AnomalyStatusPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.horizontalStretchFactor: 4
                    Layout.minimumWidth: 360
                    Layout.preferredWidth: 440
                }
            }

            EventLogPanel {
                Layout.fillWidth: true
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
