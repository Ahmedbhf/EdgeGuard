import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

Rectangle {
    id: root
    height: 92
    radius: 16
    color: Theme.panel
    border.color: Theme.borderSoft
    border.width: 1

    function compactPortLabel(label) {
        if (!label)
            return ""

        var parts = label.split(" - ")
        if (parts.length < 2)
            return label

        var port = parts[0].trim()
        var detail = parts.slice(1).join(" - ").replace(/\s*\([^)]*\)\s*$/, "").trim()

        if (detail.indexOf("Communications Port") !== -1 || detail.indexOf("Virtual COM Port") !== -1)
            detail = "USB Device"

        return detail.length > 0 ? port + " (" + detail + ")" : port
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 8

        Label {
            text: "Machine"
            font.pixelSize: 12
            color: Theme.muted
        }

        ComboBox {
            id: portCombo
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 44
            model: dataModel.availablePorts
            enabled: model.length > 0
            currentIndex: {
                for (var i = 0; i < count; ++i) {
                    if (dataModel.portNameAt(i) === dataModel.selectedPort)
                        return i
                }
                return count > 0 ? 0 : -1
            }

            displayText: currentIndex >= 0 ? root.compactPortLabel(currentText) : "Select serial device"

            onActivated: dataModel.setSelectedPort(dataModel.portNameAt(index))

            contentItem: Text {
                text: portCombo.displayText
                width: Math.max(0, portCombo.width - leftPadding - rightPadding)
                color: Theme.text
                verticalAlignment: Text.AlignVCenter
                leftPadding: 14
                rightPadding: 36
                elide: Text.ElideRight
                font.pixelSize: 14
            }

            background: Rectangle {
                radius: 12
                color: Theme.panel2
                border.color: Theme.borderSoft
                border.width: 1
            }

            indicator: Canvas {
                x: portCombo.width - width - 12
                y: (portCombo.height - height) / 2
                width: 12
                height: 8
                contextType: "2d"

                onPaint: {
                    context.reset()
                    context.moveTo(0, 0)
                    context.lineTo(width, 0)
                    context.lineTo(width / 2, height)
                    context.closePath()
                    context.fillStyle = Theme.text
                    context.fill()
                }
            }

            popup: Popup {
                y: portCombo.height + 4
                width: portCombo.width
                padding: 4

                background: Rectangle {
                    radius: 12
                    color: Theme.panel2
                    border.color: Theme.borderSoft
                    border.width: 1
                }

                contentItem: ListView {
                    clip: true
                    implicitHeight: Math.min(contentHeight, 240)
                    model: portCombo.popup.visible ? portCombo.delegateModel : null
                    currentIndex: portCombo.highlightedIndex
                }
            }

            delegate: ItemDelegate {
                width: portCombo.width
                text: root.compactPortLabel(modelData)
                highlighted: portCombo.highlightedIndex === index
                onClicked: {
                    dataModel.setSelectedPort(dataModel.portNameAt(index))
                    portCombo.popup.close()
                }

                background: Rectangle {
                    radius: 10
                    color: highlighted ? Theme.panel : "transparent"
                }
            }
        }
    }
}
