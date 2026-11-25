import QtQuick
import QtQuick.Controls

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

    Column {
        id: taskLines
        anchors.top: heading.bottom
        anchors.topMargin: page.theme.gap
        width: parent.width
        Row {
            width: parent.width
            spacing: page.theme.gap
            TextField {
                id: taskTitle
                width: Math.max(0, parent.width - addTaskButton.width - parent.spacing)
                placeholderText: qsTr("New task")
                Accessible.name: qsTr("New task")
                onAccepted: addTaskButton.clicked()
            }
            Button {
                id: addTaskButton
                text: qsTr("Add task")
                enabled: taskTitle.text.trim().length > 0
                onClicked: {
                    if (todayModel.addTask(taskTitle.text))
                        taskTitle.clear()
                }
            }
        }
        Repeater {
            model: todayModel.lines
            delegate: CheckBox {
                required property var modelData
                width: taskLines.width
                text: modelData.title
                checked: modelData.completed
                font.family: page.theme.font
                font.pixelSize: page.theme.bodySize
                // The stored Item controls the check state, including when a
                // write fails; clicking must not break the checked binding.
                nextCheckState: function() { return checkState }
                onClicked: todayModel.setTaskCompleted(modelData.id, !modelData.completed)
                indicator: Rectangle {
                    x: 0
                    y: (parent.height - height) / 2
                    width: 18
                    height: 18
                    radius: page.theme.radiusControl / 2
                    color: parent.checked ? page.theme.accent : page.theme.panel
                    border.color: parent.checked ? page.theme.accent : page.theme.muted
                    Text {
                        anchors.centerIn: parent
                        text: parent.parent.checked ? "✓" : ""
                        color: page.theme.panel
                        font.family: page.theme.font
                        font.pixelSize: page.theme.bodySize
                    }
                }
                contentItem: Text {
                    leftPadding: 26
                    text: parent.text
                    color: page.theme.panelText
                    font: parent.font
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }

    Flickable {
        id: writingArea
        anchors.top: taskLines.bottom
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
