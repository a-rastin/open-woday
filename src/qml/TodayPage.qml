import QtQuick

Item {
    id: page
    required property Theme theme

    Column {
        id: heading
        width: parent.width
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

    Flickable {
        id: writingArea
        anchors.top: heading.bottom
        anchors.topMargin: page.theme.gap
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: errorText.top
        contentWidth: width
        contentHeight: Math.max(height, editor.implicitHeight)
        clip: true

        TextEdit {
            id: editor
            width: writingArea.width
            height: Math.max(writingArea.height, implicitHeight)
            color: page.theme.panelText
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            text: todayModel.dayNoteText
            Accessible.name: qsTr("Day note")
            onTextChanged: {
                if (activeFocus && text !== todayModel.dayNoteText)
                    todayModel.setDayNoteText(text)
            }
        }
    }

    Text {
        id: errorText
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: text.length > 0
        text: todayModel.dayNoteError
        color: page.theme.accent
        font.family: page.theme.font
        font.pixelSize: page.theme.bodySize
        wrapMode: Text.Wrap
    }
}
