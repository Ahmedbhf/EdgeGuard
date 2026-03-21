import QtQuick
import EdgeGuard

Rectangle {
    id: root

    property string text: ""
    property string tone: "neutral"

    implicitWidth: label.implicitWidth + 22
    implicitHeight: 28
    radius: 999

    color: Theme.panel2
    border.width: 1
    border.color: {
        if (tone === "ok") return Qt.darker(Theme.ok, 1.8)
        if (tone === "warning") return Qt.darker(Theme.warning, 1.1)
        if (tone === "fault") return Qt.darker(Theme.fault, 1.8)
        return Theme.border
    }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: {
            if (root.tone === "ok") return Theme.ok
            if (root.tone === "warning") return Theme.warning
            if (root.tone === "fault") return Theme.fault
            return Theme.muted
        }
        font.pixelSize: 13
        font.bold: true
    }
}
