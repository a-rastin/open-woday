import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ColumnLayout {
    id: root

    required property Theme theme
    required property var taskData
    property int depth: 0
    property string selectedTaskId: ""
    property var expandedIds: ({})
    property bool selected: false
    property bool editing: false
    property bool addingSubtask: false

    readonly property string taskId: taskData && taskData.id !== undefined ? taskData.id : ""
    readonly property string title: taskData && taskData.title !== undefined ? taskData.title : ""
    readonly property bool completed: Boolean(taskData && taskData.completed)
    readonly property int priority: taskData && taskData.priority !== undefined ? taskData.priority : 0
    readonly property int durationMin: taskData && taskData.durationMin !== undefined ? taskData.durationMin : 0
    readonly property string dueTime: taskData && taskData.dueTime !== undefined ? taskData.dueTime : ""
    readonly property string folderName: taskData && taskData.folderName !== undefined ? taskData.folderName : ""
    readonly property var tags: taskData && taskData.tags !== undefined ? taskData.tags : []
    readonly property int subtaskDone: taskData && taskData.subtaskDone !== undefined ? taskData.subtaskDone : 0
    readonly property int subtaskTotal: taskData && taskData.subtaskTotal !== undefined ? taskData.subtaskTotal : 0
    readonly property var subtasks: taskData && taskData.subtasks !== undefined ? taskData.subtasks : []

    readonly property bool isSelected: selected
        || (selectedTaskId.length > 0 && selectedTaskId === taskId)
    readonly property bool expanded: Boolean(expandedIds && expandedIds[taskId])
    readonly property bool hasSubtasks: subtaskTotal > 0 && subtasks && subtasks.length > 0

    signal selectRequested(string taskId)
    signal toggleCompletedRequested(string taskId, bool completed)
    signal renameRequested(string taskId, string newTitle)
    signal deleteRequested(string taskId)
    signal toggleExpandRequested(string taskId)
    signal addSubtaskRequested(string parentId, string title, var callback)

    function beginRename() {
        editing = true
        renameField.text = root.title
        renameField.forceActiveFocus()
        renameField.selectAll()
    }

    function commitRename() {
        if (!editing)
            return
        const trimmed = renameField.text.trim()
        editing = false
        if (trimmed.length > 0 && trimmed !== root.title)
            root.renameRequested(root.taskId, trimmed)
    }

    function cancelRename() {
        editing = false
        renameField.text = root.title
    }

    function beginAddSubtask() {
        addingSubtask = true
        newSubtaskField.text = ""
        newSubtaskField.forceActiveFocus()
    }

    function submitAddSubtask() {
        const trimmed = newSubtaskField.text.trim()
        if (trimmed.length === 0)
            return
        root.addSubtaskRequested(root.taskId, trimmed, (succeeded) => {
            if (!newSubtaskField)
                return
            if (succeeded)
                newSubtaskField.text = ""
            newSubtaskField.forceActiveFocus()
        })
    }

    function cancelAddSubtask() {
        addingSubtask = false
        newSubtaskField.text = ""
    }

    spacing: 2

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: Math.max(38, contentRow.implicitHeight + 8)
        radius: root.theme.radiusControl
        color: root.isSelected
            ? root.theme.chip
            : (rowHover.hovered ? Qt.rgba(0, 0, 0, 0.03) : "transparent")
        border.color: selectButton.visualFocus
            ? root.theme.accent
            : (root.isSelected ? root.theme.border : "transparent")
        border.width: (selectButton.visualFocus || root.isSelected) ? 1 : 0

        HoverHandler {
            id: rowHover
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            enabled: !root.editing
            onClicked: root.selectRequested(root.taskId)
        }

        RowLayout {
            id: contentRow
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            anchors.leftMargin: 6
            anchors.rightMargin: 4
            spacing: root.theme.gap * 0.75

            Checkbox {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 5
                theme: root.theme
                checked: root.completed
                priority: root.priority
                accessibleName: qsTr("Complete %1").arg(root.title)
                onCheckToggled: root.toggleCompletedRequested(root.taskId, !root.completed)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Item {
                    Layout.fillWidth: true
                    implicitHeight: Math.max(selectButton.implicitHeight, renameField.implicitHeight, 26)

                    AbstractButton {
                        id: selectButton
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        implicitHeight: Math.max(titleLabel.implicitHeight, 26)
                        visible: !root.editing
                        padding: 0
                        text: root.title
                        focusPolicy: Qt.StrongFocus
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: root.title.length > 0
                            ? qsTr("Select %1").arg(root.title)
                            : qsTr("Select task")
                        onClicked: root.selectRequested(root.taskId)
                        onDoubleClicked: root.beginRename()
                        Keys.onReturnPressed: (event) => {
                            event.accepted = true
                            selectButton.clicked()
                        }
                        Keys.onEnterPressed: (event) => {
                            event.accepted = true
                            selectButton.clicked()
                        }

                        background: Rectangle {
                            radius: root.theme.radiusControl - 2
                            color: "transparent"
                        }

                        contentItem: Text {
                            id: titleLabel
                            text: root.title
                            color: root.completed ? root.theme.muted : root.theme.panelText
                            font.family: root.theme.font
                            font.pixelSize: root.depth > 0 ? root.theme.bodySize - 1 : root.theme.bodySize
                            font.strikeout: root.completed
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    TextField {
                        id: renameField
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.editing
                        font.family: root.theme.font
                        font.pixelSize: root.depth > 0 ? root.theme.bodySize - 1 : root.theme.bodySize
                        Accessible.name: qsTr("Rename task")
                        onAccepted: {
                            root.commitRename()
                            if (selectButton)
                                selectButton.forceActiveFocus(Qt.TabFocusReason)
                        }
                        Keys.onEscapePressed: (event) => {
                            event.accepted = true
                            root.cancelRename()
                            if (selectButton)
                                selectButton.forceActiveFocus(Qt.TabFocusReason)
                        }
                        onActiveFocusChanged: {
                            if (!activeFocus && root.editing)
                                root.commitRename()
                        }
                    }
                }

                ItemBadges {
                    Layout.fillWidth: true
                    visible: !root.editing
                        && (root.dueTime.length > 0 || root.priority > 0
                            || root.durationMin > 0 || root.folderName.length > 0
                            || root.subtaskTotal > 0
                            || (root.tags && root.tags.length > 0))
                    theme: root.theme
                    leadingText: root.dueTime
                    leadingAccessibleName: root.dueTime.length > 0 ? qsTr("Due time: %1").arg(root.dueTime) : ""
                    priority: root.priority
                    durationMin: root.durationMin
                    folderName: root.folderName
                    tags: root.tags
                    showSubtasks: true
                    subtaskDone: root.subtaskDone
                    subtaskTotal: root.subtaskTotal
                    subtaskExpanded: root.expanded
                    onSubtaskToggleRequested: root.toggleExpandRequested(root.taskId)
                }
            }

            IconButton {
                Layout.alignment: Qt.AlignTop
                theme: root.theme
                iconKind: "add"
                toolTipText: qsTr("Add subtask")
                visible: !root.editing
                opacity: rowHover.hovered || root.isSelected || selectButton.activeFocus || activeFocus ? 1 : 0.35
                onClicked: root.beginAddSubtask()
            }

            IconButton {
                Layout.alignment: Qt.AlignTop
                theme: root.theme
                iconKind: "edit"
                toolTipText: qsTr("Rename task")
                visible: !root.editing
                opacity: rowHover.hovered || root.isSelected || selectButton.activeFocus || activeFocus ? 1 : 0.35
                onClicked: root.beginRename()
            }

            IconButton {
                Layout.alignment: Qt.AlignTop
                theme: root.theme
                iconKind: "delete"
                destructive: true
                toolTipText: qsTr("Delete task")
                visible: !root.editing
                opacity: rowHover.hovered || root.isSelected || selectButton.activeFocus || activeFocus ? 1 : 0.35
                onClicked: root.deleteRequested(root.taskId)
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 24
        spacing: 2
        visible: (root.expanded && root.hasSubtasks) || root.addingSubtask

        Loader {
            Layout.fillWidth: true
            active: root.expanded && root.hasSubtasks
            sourceComponent: Component {
                ColumnLayout {
                    spacing: 2

                    Repeater {
                        model: root.subtasks

                        delegate: Loader {
                            id: childLoader
                            required property var modelData
                            Layout.fillWidth: true

                            function loadChild() {
                                setSource("TaskRow.qml", {
                                    theme: root.theme,
                                    taskData: modelData,
                                    depth: root.depth + 1,
                                    selectedTaskId: root.selectedTaskId,
                                    expandedIds: root.expandedIds
                                })
                            }

                            Component.onCompleted: loadChild()
                            onModelDataChanged: loadChild()

                            Binding {
                                target: childLoader.item
                                property: "selectedTaskId"
                                value: root.selectedTaskId
                                when: childLoader.item !== null
                            }

                            Binding {
                                target: childLoader.item
                                property: "expandedIds"
                                value: root.expandedIds
                                when: childLoader.item !== null
                            }

                            Connections {
                                target: childLoader.item
                                function onSelectRequested(id) { root.selectRequested(id) }
                                function onToggleCompletedRequested(id, nextCompleted) {
                                    root.toggleCompletedRequested(id, nextCompleted)
                                }
                                function onRenameRequested(id, newTitle) {
                                    root.renameRequested(id, newTitle)
                                }
                                function onDeleteRequested(id) { root.deleteRequested(id) }
                                function onToggleExpandRequested(id) { root.toggleExpandRequested(id) }
                                function onAddSubtaskRequested(parentId, subtaskTitle, callback) {
                                    root.addSubtaskRequested(parentId, subtaskTitle, callback)
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 2
            Layout.bottomMargin: 2
            spacing: root.theme.gap / 2
            visible: root.addingSubtask

            TextField {
                id: newSubtaskField
                Layout.fillWidth: true
                placeholderText: qsTr("Subtask title")
                font.family: root.theme.font
                font.pixelSize: root.theme.bodySize - 1
                Accessible.name: qsTr("Subtask title")
                onAccepted: root.submitAddSubtask()
                Keys.onEscapePressed: (event) => {
                    event.accepted = true
                    root.cancelAddSubtask()
                }
            }

            Button {
                text: qsTr("Add")
                enabled: newSubtaskField.text.trim().length > 0
                onClicked: root.submitAddSubtask()
            }

            IconButton {
                theme: root.theme
                iconKind: "close"
                toolTipText: qsTr("Cancel adding subtask")
                onClicked: root.cancelAddSubtask()
            }
        }
    }
}
