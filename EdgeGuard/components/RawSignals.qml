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
            // VIBRATION (RMS) CARD
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
                            text: "Vibration (RMS)"
                            color: Theme.text
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: {
                                var totalSamples = vibChart.values.length + vibChart.discardedSamples
                                var inBuffer = vibChart.values.length
                                var display = Math.min(inBuffer, vibChart.displayPoints)
                                return "Samples: " + totalSamples.toLocaleString(Qt.locale(), "d") +
                                       " | Buffer: " + inBuffer +
                                       " | View: " + display
                            }
                            color: Theme.muted
                            font.pixelSize: 11
                        }
                    }

                    SignalChart {
                        id: vibChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        unit: "mg"
                        displayPoints: 60
                             // Dynamic max based on data
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
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: {
                                var totalSamples = tempChart.values.length + tempChart.discardedSamples
                                var inBuffer = tempChart.values.length
                                var display = Math.min(inBuffer, tempChart.displayPoints)
                                return "Samples: " + totalSamples.toLocaleString(Qt.locale(), "d") +
                                       " | Buffer: " + inBuffer +
                                       " | View: " + display
                            }
                            color: Theme.muted
                            font.pixelSize: 11
                        }
                    }

                    SignalChart {
                        id: tempChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        unit: "°C"
                        displayPoints: 60
                          // Dynamic max based on data
                        lineColor: "#F59E0B"
                    }
                }
            }
        }

        // ===== CONNECT TO C++ MODEL =====
        Component.onCompleted: {
            vibChart.values = dataModel.vibrationValues
            tempChart.values = dataModel.temperatureValues

            dataModel.vibrationValuesChanged.connect(function() {
                vibChart.values = dataModel.vibrationValues
            })

            dataModel.temperatureValuesChanged.connect(function() {
                tempChart.values = dataModel.temperatureValues
            })
        }
    }
}
