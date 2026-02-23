import QtQuick
import EdgeGuard

Rectangle {
    id: root

    property string text: "Chip"
    property string stateType: "neutral"   // ok | warning | fault | neutral

    height: 28
    radius: 999
    implicitWidth: label.implicitWidth + 22

    color: Theme.panel2
    border.width: 1
    border.color: borderColor

    property color textColor: {
        if (stateType === "ok") return Theme.ok
        if (stateType === "warning") return Theme.warning
        if (stateType === "fault") return Theme.fault
        return Theme.muted
    }

    property color borderColor: {
        if (stateType === "ok") return Qt.darker(Theme.ok, 1.8)
        if (stateType === "warning") return Qt.darker(Theme.warning, 1.1)
        if (stateType === "fault") return Qt.darker(Theme.fault, 1.8)
        return Theme.border
    }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.textColor
        font.pixelSize: 13
        font.bold: true
    }
}
