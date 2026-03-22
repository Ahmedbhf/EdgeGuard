import QtQuick
import QtQuick.Controls
import EdgeGuard

ApplicationWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    color: Theme.bg
    visibility: Window.Maximized

    StackView {
        id: appStackView
        anchors.fill: parent
        initialItem: "SetupPage.qml"
    }
}
