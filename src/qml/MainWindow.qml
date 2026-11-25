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
        id: sidebar
        anchors.left: parent.left
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        theme: window.theme
    }

    CentrePanel {
        anchors.left: sidebar.right
        anchors.leftMargin: window.theme.gap
        anchors.right: parent.right
        anchors.rightMargin: window.theme.gap
        anchors.top: topBar.bottom
        anchors.topMargin: window.theme.gap
        anchors.bottom: parent.bottom
        anchors.bottomMargin: window.theme.gap
        theme: window.theme
    }
}
