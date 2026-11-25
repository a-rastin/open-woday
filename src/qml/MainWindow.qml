import QtQuick

Window {
    id: window

    width: 800
    height: 600
    visible: true
    title: "open-woday"

    readonly property Theme theme: Theme {}

    // Narrowest centre surface we are willing to show, in logical pixels.
    readonly property int minCentreWidth: 360

    // Horizontal space the layout reserves around the centre panel. The centre
    // always keeps one `gap` on its left and one on its right (`centreGutter`).
    // While the RightRail is visible it also reserves its own width plus the
    // `gap` between the rail and the window's right edge (`rightRailFootprint`);
    // while the Sidebar is visible it reserves its width on the left
    // (`sidebarFootprint`). Deriving these from the panels keeps the thresholds
    // correct if either panel width changes, instead of duplicating the panels'
    // `7 * titleSize` width formula here.
    readonly property int centreGutter: 2 * theme.gap
    readonly property int rightRailFootprint: rightRail.width + theme.gap
    readonly property int sidebarFootprint: sidebar.width

    // Refuse to shrink below the fully-folded frame: at this floor the centre is
    // exactly `minCentreWidth`. Keep it *low* — below both fold thresholds — so
    // the Sidebar-then-RightRail sequence stays reachable while resizing down;
    // raising the floor to protect the centre would make the rail fold moot.
    minimumWidth: minCentreWidth + centreGutter

    // Each fold fires at the width where keeping the still-visible panels would
    // squeeze the centre below `minCentreWidth`. Adding the reserved footprints
    // back onto the centre minimum derives both thresholds from the centre, the
    // panel widths and every applicable gap, so the centre is never narrower
    // than `minCentreWidth` at any width — not only at the floor. The higher
    // threshold folds the Sidebar first, the lower one the RightRail second.
    // With the current tokens: floor 384, rail fold 592, sidebar fold 788. The
    // centre equals `minCentreWidth` exactly at each fold threshold and at the
    // floor, and is wider everywhere between, so no visible side panel ever
    // squeezes it below its minimum.
    readonly property int rightRailFoldWidth: minCentreWidth + centreGutter + rightRailFootprint
    readonly property int sidebarFoldWidth: rightRailFoldWidth + sidebarFootprint

    readonly property bool showSidebar: width >= sidebarFoldWidth
    readonly property bool showRightRail: width >= rightRailFoldWidth

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
        // These `gap` margins (plus the rail's own right-margin `gap`) are the
        // ones reserved by the fold thresholds above, so the centre always
        // encloses at least `minCentreWidth` — while a side panel is still
        // visible, and once both have folded at the window minimum.
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
