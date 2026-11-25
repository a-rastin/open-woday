import QtQuick

Item {
    id: topBar

    required property Theme theme

    readonly property string modifier: Qt.platform.os === "osx" ? "⌘" : "Ctrl"

    // Below this width the title and the trailing actions crowd each other, so
    // the "Quick add" caption folds away and only its shortcut chip remains.
    // The title also elides, so no font or translation can push it under the
    // chip: the bar stays legible at the window's minimum width and across both
    // panel-fold thresholds.
    readonly property int compactWidth: 480
    readonly property bool compact: width < compactWidth

    implicitHeight: theme.titleSize + 2 * theme.gap

    Rectangle {
        anchors.fill: parent
        color: topBar.theme.sidebar
    }

    Text {
        // Cap the title at the space left of the trailing actions and elide
        // rather than clip into them when the bar is narrow.
        anchors.left: parent.left
        anchors.leftMargin: topBar.theme.gap
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(0, actions.x - 2 * topBar.theme.gap)
        elide: Text.ElideRight
        text: "Day planner"
        color: topBar.theme.sidebarText
        font.family: topBar.theme.font
        font.pixelSize: topBar.theme.titleSize
    }

    Row {
        id: actions
        anchors.right: parent.right
        anchors.rightMargin: topBar.theme.gap
        anchors.verticalCenter: parent.verticalCenter
        spacing: topBar.theme.gap

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: !topBar.compact
            text: "Quick add"
            color: topBar.theme.sidebarMuted
            font.family: topBar.theme.font
            font.pixelSize: topBar.theme.bodySize
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: shortcutHint.implicitWidth + 2 * topBar.theme.gap
            implicitHeight: shortcutHint.implicitHeight + topBar.theme.gap
            radius: topBar.theme.radiusChip
            color: topBar.theme.chip

            Text {
                id: shortcutHint
                anchors.centerIn: parent
                text: topBar.modifier + " K"
                color: topBar.theme.panelText
                font.family: topBar.theme.font
                font.pixelSize: topBar.theme.bodySize
            }
        }
    }
}
