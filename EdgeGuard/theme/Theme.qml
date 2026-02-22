pragma Singleton
import QtQuick
QtObject {

    // COLORS
    readonly property color bg: "#09090b"
    readonly property color panel: "#111113"
    readonly property color panel2: "#16161a"
    readonly property color border: "#27272a"
    readonly property color borderSoft: "#202024"

    readonly property color text: "#fafafa"

    readonly property color muted: "#a1a1aa"
    readonly property color muted2: "#71717a"

    readonly property color accent: "#e4e4e7"
    readonly property color accentText: "#09090b"

    readonly property color ok: "#86efac"
    readonly property color warning: "#fcd34d"
    readonly property color fault: "#fca5a5"

    // RADIUS
    readonly property int radiusLg: 14
    readonly property int radiusMd: 12
    readonly property int radiusSm: 10

    // SPACING
    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16
}
