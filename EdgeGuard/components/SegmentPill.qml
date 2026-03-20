import QtQuick
import QtQuick.Controls
import EdgeGuard

Rectangle {
    id: pill
    radius: 10
    color: active ? activeFillColor : "transparent"
    border.color: active ? activeBorderColor : "transparent"
    border.width: active ? 1 : 0
    implicitHeight: 32

    property string text: ""
    property bool active: false
    property color activeFillColor: Theme.panel
    property color activeBorderColor: Theme.borderSoft
    property color activeTextColor: Theme.text

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
        color: pill.active ? pill.activeTextColor : Theme.text
    }
}
