import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

ComboBox {
    id: root

    implicitWidth: 96
    implicitHeight: 34
    font.pixelSize: 13
    selectTextByMouse: false

    contentItem: Label {
        leftPadding: Theme.spaceMd
        rightPadding: Theme.spaceLg + 12
        text: root.displayText
        color: Theme.text
        font: root.font
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
    }

    indicator: Canvas {
        width: 10
        height: 6
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Theme.spaceMd

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = Theme.muted
            ctx.lineWidth = 1.6
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.moveTo(1, 1)
            ctx.lineTo(width / 2, height - 1)
            ctx.lineTo(width - 1, 1)
            ctx.stroke()
        }
    }

    background: Rectangle {
        radius: Theme.radiusSm
        color: root.pressed || root.popup.visible ? Theme.panel3 : Theme.panel2
        border.width: 1
        border.color: root.visualFocus || root.popup.visible ? Theme.primary : Theme.borderSoft
    }

    delegate: ItemDelegate {
        required property var modelData
        required property int index

        width: ListView.view ? ListView.view.width : root.width
        height: 34
        highlighted: root.highlightedIndex === index

        contentItem: Label {
            leftPadding: Theme.spaceMd
            rightPadding: Theme.spaceMd
            text: modelData
            color: highlighted ? Theme.text : Theme.text
            font.pixelSize: 13
            font.weight: highlighted ? Font.DemiBold : Font.Medium
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: Theme.radiusSm
            color: highlighted ? Theme.panel3 : "transparent"
            border.width: highlighted ? 1 : 0
            border.color: highlighted ? Theme.borderSoft : "transparent"
        }
    }

    popup: Popup {
        y: root.height + Theme.spaceXs
        width: Math.max(root.width, 96)
        padding: Theme.spaceXs

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            spacing: 2
        }

        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.panel
            border.width: 1
            border.color: Theme.borderSoft
        }
    }
}
