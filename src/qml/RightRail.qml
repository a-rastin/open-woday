import QtQuick

// The future Inbox and detail surface. It remains a clean frame with no live
// Item data until the corresponding product phase.
Rectangle {
    id: rightRail

    required property Theme theme

    implicitWidth: 7 * theme.titleSize
    width: implicitWidth

    color: rightRail.theme.panel
    radius: rightRail.theme.radiusPanel

    // Establish the shared panel typography without adding frame copy or live
    // Item content to the future Inbox and detail surface.
    Text {
        anchors.fill: parent
        text: ""
        color: rightRail.theme.panelText
        font.family: rightRail.theme.font
        font.pixelSize: rightRail.theme.bodySize
    }
}
