import QtQuick
import QtQuick.Controls

Rectangle {
    id: pill
    radius: 10
    color: active ? Qt.rgba(1,1,1,0.06) : "transparent"
    border.color: active ? Qt.rgba(1,1,1,0.14) : "transparent"
    border.width: active ? 1 : 0
    implicitHeight: 32

    property string text: ""
    property bool active: false
    property color activeTextColor: "#E6EDF3"

    signal clicked()

    MouseArea {
        anchors.fill: parent
        onClicked: pill.clicked()
        cursorShape: Qt.PointingHandCursor
    }

    Text {
        anchors.centerIn: parent
        text: pill.text
        font.pixelSize: 12
        font.weight: Font.DemiBold
        color: pill.active ? pill.activeTextColor : "#8B949E"
    }
}
