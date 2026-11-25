import QtQuick

Rectangle {
    id: centrePanel

    required property Theme theme

    color: centrePanel.theme.panel
    radius: centrePanel.theme.radiusPanel

    TodayPage {
        anchors.fill: parent
        anchors.margins: centrePanel.theme.gap * 2
        theme: centrePanel.theme
    }
}
