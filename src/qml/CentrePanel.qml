import QtQuick

// The clean surface later pages render into. It stays empty of product content
// while establishing the shared panel colour and radius.
Rectangle {
    id: centrePanel

    required property Theme theme

    color: centrePanel.theme.panel
    radius: centrePanel.theme.radiusPanel
}
