import QtQuick

Window {
    id: window

    width: 800
    height: 600
    visible: true
    title: "open-woday"

    readonly property Theme theme: Theme {}

    // Preserve the centre surface by folding the Sidebar before the RightRail.
    readonly property bool showSidebar: width >= 720
    readonly property bool showRightRail: width >= 560

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
        visible: window.showSidebar
        theme: window.theme
    }

    RightRail {
        id: rightRail
        anchors.right: parent.right
        anchors.rightMargin: window.theme.gap
        anchors.top: topBar.bottom
        anchors.topMargin: window.theme.gap
        anchors.bottom: parent.bottom
        anchors.bottomMargin: window.theme.gap
        visible: window.showRightRail
        theme: window.theme
    }

    CentrePanel {
        anchors.left: window.showSidebar ? sidebar.right : parent.left
        anchors.leftMargin: window.theme.gap
        anchors.right: window.showRightRail ? rightRail.left : parent.right
        anchors.rightMargin: window.theme.gap
        anchors.top: topBar.bottom
        anchors.topMargin: window.theme.gap
        anchors.bottom: parent.bottom
        anchors.bottomMargin: window.theme.gap
        theme: window.theme
    }
}
