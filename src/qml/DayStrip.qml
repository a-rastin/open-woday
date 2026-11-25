import QtQuick

ListView {
    id: strip
    required property Theme theme
    orientation: ListView.Horizontal
    clip: true
    spacing: theme.gap
    model: todayModel
    boundsBehavior: Flickable.StopAtBounds

    delegate: Rectangle {
        id: cell
        required property string date
        required property string weekday
        required property string dayNumber
        required property bool isToday
        required property bool isSelected
        width: strip.theme.dayStripCell.width
        height: strip.theme.dayStripCell.height
        radius: strip.theme.radiusControl
        color: isToday ? strip.theme.accent
                       : (isSelected ? strip.theme.chip : strip.theme.panel)
        border.color: isSelected && !isToday ? strip.theme.muted : color
        border.width: isSelected && !isToday ? 1 : 0

        Column {
            anchors.centerIn: parent
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: cell.weekday
                color: cell.isToday ? strip.theme.panel : strip.theme.muted
                font.family: strip.theme.font
                font.pixelSize: strip.theme.bodySize
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: cell.dayNumber
                color: cell.isToday ? strip.theme.panel : strip.theme.panelText
                font.family: strip.theme.font
                font.pixelSize: strip.theme.bodySize
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: todayModel.selectDate(cell.date)
        }
    }
}
