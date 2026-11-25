import QtQuick

Item {
    id: page
    required property Theme theme

    Column {
        anchors.fill: parent
        spacing: page.theme.gap

        Text {
            width: parent.width
            text: qsTr("Today")
            color: page.theme.panelText
            font.family: page.theme.font
            font.pixelSize: page.theme.titleSize
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: todayModel.selectedDateLabel
            color: page.theme.muted
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            elide: Text.ElideRight
        }

        DayStrip {
            width: parent.width
            height: page.theme.dayStripCell.height
            theme: page.theme
        }

        Text {
            width: parent.width
            text: todayModel.summary
            color: page.theme.panelText
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            elide: Text.ElideRight
        }
    }
}
