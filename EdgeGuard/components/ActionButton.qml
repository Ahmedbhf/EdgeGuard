import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import EdgeGuard

Button {
    id: root
    hoverEnabled: false
    focusPolicy: Qt.NoFocus
    property string toolTip: ""
    property bool primary: false

    height: 40
    padding: 16

    implicitWidth: label.implicitWidth + padding * 2

    background: Rectangle {
        radius: 14
        color: root.primary
               ? "#e4e4e7"
               : (root.hovered ? "#18181d" : Theme.panel2)
        border.width: root.primary ? 0 : 1
        border.color: Theme.border
    }

    contentItem: Text {
        id: label
        text: root.text
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: root.primary ? Theme.accentText : Theme.text
        font.pixelSize: 14
        font.bold: true
    }
}
