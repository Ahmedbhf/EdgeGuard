import QtQuick
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root

    property string text: ""
    property bool active: false
    property color activeColor: Theme.ok
    property real borderHighlight: 1.15

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredHeight: 52
    radius: 12
    color: root.active ? root.activeColor : Theme.panel
    border.color: root.active ? Qt.lighter(root.activeColor, root.borderHighlight) : Theme.borderSoft
    border.width: 1

    Text {
        anchors.centerIn: parent
        text: root.text
        color: root.active ? Theme.primaryFg : Theme.muted
        opacity: root.active ? 1.0 : 0.7
        font.pixelSize: 14
        font.weight: Font.DemiBold
    }
}
