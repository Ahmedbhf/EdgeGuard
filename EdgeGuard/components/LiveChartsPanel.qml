import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

PanelCard {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    title: "Live Charts"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
            radius: 14
            color: Theme.panel2
            border.color: Theme.borderSoft
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "RMS Vibration (mg)"
                    color: Theme.lightMode ? "#4b5563" : "#d2d7df"
                    font.weight: Font.Bold
                }

                LiveTrendChart {
                    id: rmsChart
                    // This chart uses the rolling vibration history prepared by the backend.
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    values: appController.vibrationValues
                    unit: "mg"
                    showUnitLabel: false
                    displayPoints: 120
                    sampleRateHz: 20.0
                    lineColor: "#86BBFF"
                    anomalyActive: appController.state === "CRITICAL"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
            radius: 14
            color: Theme.panel2
            border.color: Theme.borderSoft
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "Temperature (\u00B0C)"
                    color: Theme.lightMode ? "#4b5563" : "#d2d7df"
                    font.weight: Font.Bold
                }

                LiveTrendChart {
                    id: temperatureChart
                    // This second chart reuses the same component with temperature-specific settings.
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    values: appController.temperatureValues
                    unit: "\u00B0C"
                    showUnitLabel: false
                    displayPoints: 120
                    sampleRateHz: 20.0
                    lineColor: "#F6AD55"
                    anomalyActive: appController.state === "CRITICAL"
                }
            }
        }
    }
}
