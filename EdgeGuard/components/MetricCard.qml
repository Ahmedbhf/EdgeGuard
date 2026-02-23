import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root

    Layout.fillWidth: true
    radius: Theme.radiusMd
    color: Theme.panel2
    border.color: Theme.borderSoft
    border.width: 1

    property string label: ""
    property string value: ""
    property string sizeVariant: "normal"

    property int valueSize: sizeVariant === "large" ? 40 : 18
    property int labelSize: sizeVariant === "large" ? 14 : 12

    implicitHeight: content.implicitHeight + Theme.spaceLg * 2

    Column {
        id: content
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: sizeVariant === "large" ? 8 : 6

        Text {
            text: root.label
            font.pixelSize: root.labelSize
            color: Theme.muted
        }

        Text {
            text: root.value
            font.pixelSize: root.valueSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
    }
}
