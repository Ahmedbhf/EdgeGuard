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

        // ===== HEADER =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Label {
                    text: "Raw Signals"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSoft
        }

        // ===== CONTENT =====
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
            spacing: 20

            // =========================
            // VIBRATION CARD
            // =========================
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

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Vibration"
                            color: Theme.text
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: {
                                var totalSamples = vibChart.values.length + vibChart.discardedSamples
                                var inBuffer = vibChart.values.length
                                var display = Math.min(inBuffer, vibChart.displayPoints)
                                return "Samples received: " + totalSamples.toLocaleString(Qt.locale(), "d") +
                                       " | In buffer: " + inBuffer +
                                       " | Display: " + display
                            }
                            color: Theme.muted
                            font.pixelSize: 12
                        }
                    }

                    SignalChart {
                        id: vibChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        unit: "g"
                        displayPoints: 60
                    }
                }
            }

            // =========================
            // TEMPERATURE CARD
            // =========================
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

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Temperature"
                            color: Theme.text
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: {
                                var totalSamples = tempChart.values.length + tempChart.discardedSamples
                                var inBuffer = tempChart.values.length
                                var display = Math.min(inBuffer, tempChart.displayPoints)
                                return "Samples received: " + totalSamples.toLocaleString(Qt.locale(), "d") +
                                       " | In buffer: " + inBuffer +
                                       " | Display: " + display
                            }
                            color: Theme.muted
                            font.pixelSize: 12
                        }
                    }

                    SignalChart {
                        id: tempChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        unit: "°C"
                        displayPoints: 60
                    }
                }
            }
        }

        // ===== CONNECT TO C++ MODEL =====
        Component.onCompleted: {
            // Initial connection - get the data from C++
            vibChart.values = dataModel.vibrationValues
            tempChart.values = dataModel.temperatureValues

            // Listen for updates from C++ model
            dataModel.vibrationValuesChanged.connect(function() {
                vibChart.values = dataModel.vibrationValues
            })

            dataModel.temperatureValuesChanged.connect(function() {
                tempChart.values = dataModel.temperatureValues
            })
        }
    }
}
