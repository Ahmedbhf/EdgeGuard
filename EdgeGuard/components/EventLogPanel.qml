import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

PanelCard {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 220
    title: "Event Log"

    ScrollView {
        anchors.fill: parent
        anchors.margins: 12
        clip: true

        TextArea {
            text: dataModel.logText
            readOnly: true
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            color: Theme.text
            font.pixelSize: 13

            background: Rectangle {
                radius: 12
                color: Theme.panel2
                border.color: Theme.borderSoft
                border.width: 1
            }
        }
    }
}
