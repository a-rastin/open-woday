import QtQuick

// The one source of visual tokens for the open-woday frame. Later surfaces
// inherit this object rather than repeating colours, spacing or typography.
QtObject {
    readonly property color canvas: "#1C1C1E"
    readonly property color sidebar: "#17181C"
    readonly property color sidebarText: "#E8E8E8"
    readonly property color sidebarMuted: "#8A8A8A"
    readonly property color panel: "#FFFFFF"
    readonly property color panelText: "#161616"
    readonly property color muted: "#757575"
    readonly property color chip: "#F5F5F5"
    readonly property color border: "#E4E4E7"
    readonly property color accent: "#E8503A"
    readonly property color priorityLow: "#3B82F6"
    readonly property color priorityMedium: "#D97706"
    readonly property color priorityHigh: accent

    readonly property real radiusPanel: 18
    readonly property real radiusChip: 999
    readonly property real radiusControl: 8
    readonly property real gap: 12

    // QML's font.family matches a single family: a comma-separated string is
    // one literal name and would never resolve to Inter. Resolve the installed
    // family once for the whole frame — Inter when available, otherwise the
    // platform's system UI font (the application's default font).
    readonly property string font: {
        if (Qt.fontFamilies().indexOf("Inter") !== -1)
            return "Inter";
        return Qt.application.font ? Qt.application.font.family : "";
    }
    readonly property int bodySize: 14
    readonly property int titleSize: 28
    readonly property size dayStripCell: Qt.size(44, 56)

    function prioritySpec(level) {
        switch (level) {
        case 3:
            return { color: priorityHigh, label: qsTr("High") }
        case 2:
            return { color: priorityMedium, label: qsTr("Medium") }
        case 1:
            return { color: priorityLow, label: qsTr("Low") }
        default:
            return { color: muted, label: qsTr("None") }
        }
    }

    function priorityColor(level) {
        return prioritySpec(level).color
    }

    function priorityLabel(level) {
        return prioritySpec(level).label
    }

    function formatDuration(minutes) {
        if (minutes <= 0)
            return ""
        const hours = Math.floor(minutes / 60)
        const mins = minutes % 60
        if (hours > 0 && mins > 0)
            return qsTr("%1 h %2 min").arg(hours).arg(mins)
        if (hours > 0)
            return qsTr("%1 h").arg(hours)
        return qsTr("%1 min").arg(mins)
    }

    readonly property var tagPalette: [
        { fill: "#E6F4EA", text: "#1E6B3A", border: "#B7DFC2" },
        { fill: "#E8F0FE", text: "#1D4ED8", border: "#BFDBFE" },
        { fill: "#F3E8FD", text: "#6D28D9", border: "#DDD6FE" },
        { fill: "#FEF3C7", text: "#92400E", border: "#FDE68A" },
        { fill: "#FCE7F3", text: "#9D174D", border: "#FBCFE8" },
        { fill: "#E0F2FE", text: "#0369A1", border: "#BAE6FD" }
    ]

    function validHexColor(colour) {
        return typeof colour === "string" && /^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$/.test(colour)
    }

    function tagPaletteEntry(tagName) {
        let hash = 0
        const text = tagName || ""
        for (let i = 0; i < text.length; ++i)
            hash = ((hash * 31) + text.charCodeAt(i)) >>> 0
        return tagPalette[hash % tagPalette.length]
    }

    function tagFillColor(tagName, customColour) {
        if (validHexColor(customColour)) {
            const c = Qt.color(customColour)
            return Qt.rgba(c.r, c.g, c.b, 0.14)
        }
        return tagPaletteEntry(tagName).fill
    }

    function tagTextColor(tagName, customColour) {
        if (validHexColor(customColour))
            return Qt.darker(Qt.color(customColour), 1.35)
        return tagPaletteEntry(tagName).text
    }

    function tagBorderColor(tagName, customColour) {
        if (validHexColor(customColour)) {
            const c = Qt.color(customColour)
            return Qt.rgba(c.r, c.g, c.b, 0.30)
        }
        return tagPaletteEntry(tagName).border
    }
}
