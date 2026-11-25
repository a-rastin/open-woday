import QtQuick

Window {
    id: window

    width: 800
    height: 600
    // Refuse to shrink below a usable centre surface: the gap margins on each
    // side of the centre panel are reserved here, so it always has room to
    // render and the TopBar contents never have to compete for width. Both
    // fold thresholds stay above this floor, keeping the Sidebar-then-RightRail
    // sequence reachable while resizing down to the minimum.
    minimumWidth: minCentreWidth + 2 * theme.gap
    visible: true
    title: "open-woday"

    readonly property Theme theme: Theme {}

    // Narrowest centre surface we are willing to show, in logical pixels.
    readonly property int minCentreWidth: 360

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
        // minimumWidth guarantees these gap margins always enclose a positive,
        // legible centre surface, even once both side panels have folded.
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
