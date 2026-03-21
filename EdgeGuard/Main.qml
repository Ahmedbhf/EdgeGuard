import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import EdgeGuard

ApplicationWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    color: Theme.bg
    visibility: Window.Maximized
    property bool showingHistoryPage: false

    ColumnLayout {
        visible: !root.showingHistoryPage
        anchors.fill: parent
        spacing: Theme.spaceLg

        DashboardHeaderBar {
            Layout.fillWidth: true
            onConnectionToggled: dataModel.toggleConnection()
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

            PortSelectorCard {
                Layout.fillWidth: true
            }

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
