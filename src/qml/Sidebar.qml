import QtQuick

// The navigation frame beside the centre page. It stays declarative and free
// of Item, Folder or Tag data until the corresponding product phase.
Item {
    id: sidebar

    required property Theme theme

    implicitWidth: 7 * theme.titleSize
    width: implicitWidth

    Rectangle {
        anchors.fill: parent
        color: sidebar.theme.sidebar
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: sidebar.theme.gap
        anchors.topMargin: sidebar.theme.gap
        spacing: 2 * sidebar.theme.gap

        Text {
            text: qsTr("Today")
            color: sidebar.theme.sidebarText
            font.family: sidebar.theme.font
            font.pixelSize: sidebar.theme.titleSize
        }

        Text {
            text: qsTr("Folders")
            color: sidebar.theme.sidebarMuted
            font.family: sidebar.theme.font
            font.pixelSize: sidebar.theme.bodySize
        }

        Text {
            text: qsTr("Tags")
            color: sidebar.theme.sidebarMuted
            font.family: sidebar.theme.font
            font.pixelSize: sidebar.theme.bodySize
        }
    }
}
