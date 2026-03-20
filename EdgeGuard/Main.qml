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

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceLg

        AppHeader {
            Layout.fillWidth: true
            onConnectionToggled: dataModel.toggleConnection()
            onExportCsvClicked: dataModel.openCsvFile()
            onRefreshClicked: dataModel.refreshPorts()
            onThemeToggleClicked: Theme.toggleMode()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16
            spacing: Theme.spaceLg

            MotorSelector {
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spaceLg
                Layout.minimumHeight: 0

                RawSignals {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.horizontalStretchFactor: 7
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: 860
                }

                FeaturesCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.horizontalStretchFactor: 5
                    Layout.minimumWidth: 420
                    Layout.preferredWidth: 520
                }

                MotorHealthStatus {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.horizontalStretchFactor: 4
                    Layout.minimumWidth: 360
                    Layout.preferredWidth: 440
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                radius: 18
                color: Theme.panel
                border.color: Theme.borderSoft
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 48
                        color: "transparent"

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            text: "Log Panel"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.text
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.borderSoft
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 12
                        clip: true

                        TextArea {
                            text: dataModel.logText
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            color: Theme.text
                            font.pixelSize: 13
                            background: Rectangle {
                                radius: 12
                                color: Theme.panel2
                                border.color: Theme.borderSoft
                                border.width: 1
                            }
                        }
                    }
                }
            }
        }
    }
}
