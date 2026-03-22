import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 16
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                text: "Live Charts"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.text
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSoft
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        values: dataModel.vibrationValues
                        unit: "mg"
                        showUnitLabel: false
                        displayPoints: 120
                        sampleRateHz: 20.0
                        lineColor: "#86BBFF"
                        anomalyActive: dataModel.state === "ANOMALY"
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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        values: dataModel.temperatureValues
                        unit: "\u00B0C"
                        showUnitLabel: false
                        displayPoints: 120
                        sampleRateHz: 20.0
                        lineColor: "#F6AD55"
                        anomalyActive: dataModel.state === "ANOMALY"
                    }
                }
            }
        }

    }
}
