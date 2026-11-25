import QtQuick

Window {
    id: window

    width: 800
    height: 600
    visible: true
    title: "open-woday"

    readonly property Theme theme: Theme {}

    color: theme.canvas

    TopBar {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        theme: window.theme
    }

    Sidebar {
        anchors.left: parent.left
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        theme: window.theme
    }
}
