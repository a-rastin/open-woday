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

    function priorityColor(level) {
        if (level === 3)
            return priorityHigh
        if (level === 2)
            return priorityMedium
        if (level === 1)
            return priorityLow
        return muted
    }

    function priorityLabel(level) {
        if (level === 3)
            return qsTr("High")
        if (level === 2)
            return qsTr("Medium")
        if (level === 1)
            return qsTr("Low")
        return qsTr("None")
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

    function isoDateOffset(days) {
        const now = new Date()
        const date = new Date(now.getFullYear(), now.getMonth(), now.getDate() + days)
        return Qt.formatDate(date, "yyyy-MM-dd")
    }

    function formatDueDate(isoDay) {
        if (!isoDay || isoDay.length !== 10)
            return ""
        if (isoDay === isoDateOffset(0))
            return qsTr("Today")
        if (isoDay === isoDateOffset(1))
            return qsTr("Tomorrow")
        if (isoDay === isoDateOffset(-1))
            return qsTr("Yesterday")
        const parts = isoDay.split("-")
        if (parts.length !== 3)
            return isoDay
        const parsed = new Date(Number(parts[0]), Number(parts[1]) - 1, Number(parts[2]))
        if (isNaN(parsed.getTime()))
            return isoDay
        return Qt.locale("C").toString(parsed, "d MMM yyyy")
    }

    function formatDueDateTime(isoDay, dueTime) {
        const dateLabel = formatDueDate(isoDay)
        if (dateLabel.length === 0)
            return ""
        if (dueTime && dueTime.length > 0)
            return qsTr("%1, %2").arg(dateLabel).arg(dueTime)
        return dateLabel
    }
}
