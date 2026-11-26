import QtQuick
import QtQuick.Controls

Item {
    id: page
    required property Theme theme
    property var expandedTasks: ({})

    function toggleTaskExpanded(taskId) {
        const next = Object.assign({}, page.expandedTasks)
        next[taskId] = !next[taskId]
        page.expandedTasks = next
    }

    function expandTask(taskId) {
        if (page.expandedTasks[taskId])
            return
        const next = Object.assign({}, page.expandedTasks)
        next[taskId] = true
        page.expandedTasks = next
    }

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
        anchors.bottom: errors.top
        contentWidth: width
        contentHeight: taskLines.height + page.theme.gap + editor.height
        clip: true
        ScrollBar.vertical: ScrollBar {}

        Column {
            id: taskLines
            width: writingArea.width
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
                delegate: Loader {
                    required property var modelData
                    width: taskLines.width
                    sourceComponent: modelData.kind === "event" ? eventLine : taskLine
                }
            }
            Component {
                id: eventLine
                Text {
                    width: taskLines.width
                    height: 40
                    text: qsTr("Event: %1").arg(parent.modelData.title)
                    color: page.theme.panelText
                    font.family: page.theme.font
                    font.pixelSize: page.theme.bodySize
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            Component {
                id: taskLine
                TaskRow {
                    width: taskLines.width
                    theme: page.theme
                    taskId: parent.modelData.id
                    title: parent.modelData.title
                    completed: parent.modelData.completed
                    priority: parent.modelData.priority !== undefined ? parent.modelData.priority : 0
                    durationMin: parent.modelData.durationMin !== undefined ? parent.modelData.durationMin : 0
                    dueTime: parent.modelData.dueTime !== undefined ? parent.modelData.dueTime : ""
                    folderName: parent.modelData.folderName !== undefined ? parent.modelData.folderName : ""
                    tags: parent.modelData.tags !== undefined ? parent.modelData.tags : []
                    subtaskDone: parent.modelData.subtaskDone !== undefined ? parent.modelData.subtaskDone : 0
                    subtaskTotal: parent.modelData.subtaskTotal !== undefined ? parent.modelData.subtaskTotal : 0
                    subtasks: parent.modelData.subtasks !== undefined ? parent.modelData.subtasks : []
                    selectedTaskId: itemModel.hasSelection ? itemModel.selectedId : ""
                    expandedIds: page.expandedTasks
                    onSelectRequested: (id) => itemModel.selectTask(id)
                    onToggleCompletedRequested: (id, nextCompleted) => itemModel.setTaskCompleted(id, nextCompleted)
                    onRenameRequested: (id, newTitle) => itemModel.renameTask(id, newTitle)
                    onDeleteRequested: (id) => itemModel.deleteTask(id)
                    onToggleExpandRequested: (id) => page.toggleTaskExpanded(id)
                    onAddSubtaskRequested: (parentId, subtaskTitle) => {
                        if (itemModel.addSubtask(parentId, subtaskTitle).length > 0)
                            page.expandTask(parentId)
                    }
                }
            }
        }

        TextEdit {
            id: editor
            y: taskLines.height + page.theme.gap
            width: writingArea.width
            height: Math.max(160, writingArea.height - y, implicitHeight)
            color: page.theme.panelText
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            readOnly: !todayModel.dayNoteEditable
            text: todayModel.dayNoteText
            Accessible.name: qsTr("Day note")
            onCursorRectangleChanged: {
                if (!activeFocus || writingArea.height <= 0)
                    return
                const cursorTop = y + cursorRectangle.y
                const cursorBottom = cursorTop + cursorRectangle.height
                if (cursorBottom > writingArea.contentY + writingArea.height)
                    writingArea.contentY = Math.min(writingArea.contentHeight - writingArea.height,
                                                    cursorBottom - writingArea.height)
                else if (cursorTop < writingArea.contentY)
                    writingArea.contentY = Math.max(0, cursorTop)
            }
            onTextChanged: {
                if (activeFocus && text !== todayModel.dayNoteText)
                    todayModel.setDayNoteText(text)
            }
            onActiveFocusChanged: {
                if (!activeFocus)
                    todayModel.flushDayNote()
            }
        }
    }

    Column {
        id: errors
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        Text {
            width: parent.width
            visible: text.length > 0
            text: todayModel.refreshError.length > 0
                  ? qsTr("Could not refresh Tasks and Events; showing the last available results: %1").arg(todayModel.refreshError)
                  : ""
            color: page.theme.accent
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            visible: text.length > 0
            text: todayModel.taskError
            color: page.theme.accent
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            visible: text.length > 0
            text: itemModel.error
            color: page.theme.accent
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            visible: text.length > 0
            text: todayModel.dayNoteError
            color: page.theme.accent
            font.family: page.theme.font
            font.pixelSize: page.theme.bodySize
            wrapMode: Text.Wrap
        }
    }
}
