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
                        text: "RMS Vibration"
                        color: Theme.text
                        font.weight: Font.DemiBold
                    }

                    LiveTrendChart {
                        id: rmsChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        unit: "mg"
                        displayPoints: 60
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
                        text: "Temperature"
                        color: Theme.text
                        font.weight: Font.DemiBold
                    }

                    LiveTrendChart {
                        id: temperatureChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        unit: "°C"
                        displayPoints: 60
                        lineColor: "#F59E0B"
                    }
                }
            }
        }

    }

    Component.onCompleted: {
        rmsChart.values = dataModel.vibrationValues
        temperatureChart.values = dataModel.temperatureValues

        dataModel.vibrationValuesChanged.connect(function() {
            rmsChart.values = dataModel.vibrationValues
        })

        dataModel.temperatureValuesChanged.connect(function() {
            temperatureChart.values = dataModel.temperatureValues
        })
    }
}
