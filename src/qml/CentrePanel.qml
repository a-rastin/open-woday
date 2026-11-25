import QtQuick

// The clean surface later pages render into. It stays empty of product content
// while establishing the shared panel colour, radius and typography.
Rectangle {
    id: centrePanel

    required property Theme theme

    color: centrePanel.theme.panel
    radius: centrePanel.theme.radiusPanel

    Text {
        anchors.fill: parent
        text: ""
        color: centrePanel.theme.panelText
        font.family: centrePanel.theme.font
        font.pixelSize: centrePanel.theme.bodySize
    }
}
