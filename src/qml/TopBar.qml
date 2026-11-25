import QtQuick

Item {
    id: topBar

    required property Theme theme

    readonly property string modifier: Qt.platform.os === "osx" ? "⌘" : "Ctrl"

    implicitHeight: theme.titleSize + 2 * theme.gap

    Rectangle {
        anchors.fill: parent
        color: topBar.theme.sidebar
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: topBar.theme.gap
        anchors.verticalCenter: parent.verticalCenter
        text: "Day planner"
        color: topBar.theme.sidebarText
        font.family: topBar.theme.font
        font.pixelSize: topBar.theme.titleSize
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: topBar.theme.gap
        anchors.verticalCenter: parent.verticalCenter
        spacing: topBar.theme.gap

        Text {
            anchors.verticalCenter: parent.verticalCenter
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
