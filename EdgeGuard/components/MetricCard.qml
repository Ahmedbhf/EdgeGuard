import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root

    Layout.fillWidth: true
    radius: 14
    color: Qt.rgba(1,1,1,0.04)
    border.color: Qt.rgba(1,1,1,0.06)
    border.width: 1

    property string label: ""
    property string value: ""
    property string sizeVariant: "normal"

    property int valueSize: sizeVariant === "large" ? 40 : 18
    property int labelSize: sizeVariant === "large" ? 14 : 12

    implicitHeight: content.implicitHeight + 32

    Column {
        id: content
        anchors.fill: parent
        anchors.margins: 16
        spacing: sizeVariant === "large" ? 8 : 6

        Text {
            text: root.label
            font.pixelSize: root.labelSize
            color: "#8B949E"
        }

        Text {
            text: root.value
            font.pixelSize: root.valueSize
            font.weight: Font.DemiBold
            color: "#E6EDF3"
        }
    }
}
