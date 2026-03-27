import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EdgeGuard

PanelCard {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    implicitHeight: 420
    title: "Key Metrics"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        ValueCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.verticalStretchFactor: 1
            label: "RMS (mg)"
            value: dataModel.rms.toFixed(2)
            sizeVariant: "hero"
        }
        ValueCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.verticalStretchFactor: 1
            label: "Machine Temp (°C)"
            value: dataModel.temp.toFixed(1)
            sizeVariant: "hero"
        }

        ValueCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.verticalStretchFactor: 1
            label: "Ambient Temp (°C)"
            value: dataModel.ambientTemp.toFixed(1)
            sizeVariant: "hero"
        }
    }
}
