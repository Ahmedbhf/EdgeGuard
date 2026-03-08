pragma Singleton
import QtQuick

QtObject {

    // MODE
    property bool lightMode: false

    function toggleMode() {
        lightMode = !lightMode
    }

    // ===== COLORS =====

    // ===== COLORS =====

    readonly property color bg: lightMode ? "#fefefe" : "#09090b"

    readonly property color panel: lightMode ? "#ffffff" : "#111113"
    readonly property color panel2: lightMode ? "#f8fafc" : "#16161a"
    readonly property color panel3: lightMode ? "#fafafa" : "#11111a"
    readonly property color border: lightMode ? "#dedede" : "#27272a"
    readonly property color borderSoft: lightMode ? "#e5e7eb" : "#202024"

    readonly property color text: lightMode ? "#0f172a" : "#fafafa"

    readonly property color muted: lightMode ? "#475569" : "#a1a1aa"
    readonly property color muted2: lightMode ? "#64748b" : "#71717a"

    readonly property color accent: lightMode ? "#111827" : "#e4e4e7"
    readonly property color accentText: lightMode ? "#ffffff" : "#09090b"

    readonly property color ok: lightMode ? "#16a34a" : "#86efac"
    readonly property color warning: lightMode ? "#d97706" : "#fcd34d"
    readonly property color fault: lightMode ? "#dc2626" : "#fca5a5"
    readonly property color primary: lightMode ? "#0865FF" : "#0865FF"
    readonly property color primaryFg: lightMode ? "#fff" : "#fff"
    // ===== RADIUS =====
    readonly property int radiusLg: 14
    readonly property int radiusMd: 12
    readonly property int radiusSm: 10

    // ===== SPACING =====
    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16
}
