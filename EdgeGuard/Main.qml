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

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceLg

        AppHeader { Layout.fillWidth: true }

        ScrollView {
            id: sv
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 6
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: "#3A3F4B"
                }
                background: Rectangle {
                    color: "transparent"
                }
            }

            ColumnLayout {
                id: page
                width: sv.availableWidth
                spacing: Theme.spaceLg


                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    spacing: Theme.spaceLg

                    MotorSelector { Layout.fillWidth: true }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 620
                        spacing: Theme.spaceLg
                        Layout.alignment: Qt.AlignTop

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.horizontalStretchFactor: 2
                            Layout.minimumWidth: 500
                            radius: 20
                            color: Theme.panel
                            border.color: Theme.borderSoft
                        }

                        FeaturesCard {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.horizontalStretchFactor: 1
                            Layout.minimumWidth: 500
                        }
                        MotorHealthStatus{

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.horizontalStretchFactor: 1
                            Layout.minimumWidth: 500
                        }

                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 320
                        radius: 18
                        color: Theme.panel
                        border.color: Theme.borderSoft
                    }

                    Item { height: 16 }
                }
            }
        }
    }
}
