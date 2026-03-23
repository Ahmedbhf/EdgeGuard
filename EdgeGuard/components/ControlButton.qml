import QtQuick
import QtQuick.Controls
import EdgeGuard

Button {
    id: root
    hoverEnabled: false
    focusPolicy: Qt.NoFocus

    property bool primary: false

    implicitHeight: 40
    padding: 12
    implicitWidth: label.implicitWidth + padding * 2

    background: Rectangle {
        radius: 14
        color: {
            if (!root.enabled)
                return root.primary ? Theme.borderSoft : Theme.panel2
            return root.primary ? Theme.primary : (root.hovered ? "#18181d" : Theme.panel2)
        }
        border.width: root.primary ? 0 : 1
        border.color: root.enabled ? Theme.border : Theme.borderSoft
    }

    contentItem: Text {
        id: label
        text: root.text
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: {
            if (!root.enabled)
                return root.primary ? Theme.primaryFg : Theme.muted
            return root.primary ? Theme.primaryFg : Theme.text
        }
        opacity: root.enabled ? 1.0 : 0.85
        font.pixelSize: 14
        font.bold: true
    }
}
