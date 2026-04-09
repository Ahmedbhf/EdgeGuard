import QtQuick
import QtQuick.Controls
import EdgeGuard

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    minimumWidth: 800
    minimumHeight: 480
    visible: true
    color: Theme.bg

    StackView {
        id: appStackView
        anchors.fill: parent
        initialItem: "SetupPage.qml"
    }
}
