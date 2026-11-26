import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ColumnLayout {
    id: root

    required property Theme theme
    required property string taskId
    required property string title
    required property bool completed
    property int priority: 0
    property int durationMin: 0
    property string dueTime: ""
    property string folderName: ""
    property var tags: []
    property int subtaskDone: 0
    property int subtaskTotal: 0
    property var subtasks: []
    property int depth: 0
    property string selectedTaskId: ""
    property var expandedIds: ({})
    property bool selected: false
    property bool editing: false
    property bool addingSubtask: false

    readonly property bool isSelected: selected
        || (selectedTaskId.length > 0 && selectedTaskId === taskId)
    readonly property bool expanded: Boolean(expandedIds && expandedIds[taskId])
    readonly property bool hasSubtasks: subtaskTotal > 0 && subtasks && subtasks.length > 0

    signal selectRequested(string taskId)
    signal toggleCompletedRequested(string taskId, bool completed)
    signal renameRequested(string taskId, string newTitle)
    signal deleteRequested(string taskId)
    signal toggleExpandRequested(string taskId)
    signal addSubtaskRequested(string parentId, string title)

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
        root.addSubtaskRequested(root.taskId, trimmed)
        newSubtaskField.text = ""
        newSubtaskField.forceActiveFocus()
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
        border.color: root.isSelected ? root.theme.border : "transparent"
        border.width: root.isSelected ? 1 : 0

        HoverHandler {
            id: rowHover
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
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
                    implicitHeight: Math.max(titleLabel.implicitHeight, renameField.implicitHeight, 26)

                    Text {
                        id: titleLabel
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !root.editing
                        text: root.title
                        color: root.completed ? root.theme.muted : root.theme.panelText
                        font.family: root.theme.font
                        font.pixelSize: root.depth > 0 ? root.theme.bodySize - 1 : root.theme.bodySize
                        font.strikeout: root.completed
                        elide: Text.ElideRight

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.selectRequested(root.taskId)
                            onDoubleClicked: root.beginRename()
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
                        onAccepted: root.commitRename()
                        Keys.onEscapePressed: (event) => {
                            event.accepted = true
                            root.cancelRename()
                        }
                        onActiveFocusChanged: {
                            if (!activeFocus && root.editing)
                                root.commitRename()
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: !root.editing && (root.dueTime.length > 0 || root.priority > 0
                        || root.durationMin > 0 || root.folderName.length > 0
                        || root.subtaskTotal > 0
                        || (root.tags && root.tags.length > 0))

                    Badge {
                        visible: root.dueTime.length > 0
                        interactive: false
                        theme: root.theme
                        text: root.dueTime
                        accessibleName: qsTr("Due time: %1").arg(text)
                        fillColor: Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.10)
                        textColor: root.theme.accent
                        borderColor: Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.22)
                        padding: 2
                        leftPadding: 8
                        rightPadding: 8
                    }

                    Badge {
                        readonly property color tone: root.theme.priorityColor(root.priority)
                        visible: root.priority > 0
                        interactive: false
                        theme: root.theme
                        text: root.theme.priorityLabel(root.priority)
                        accessibleName: qsTr("Priority: %1").arg(text)
                        fillColor: Qt.rgba(tone.r, tone.g, tone.b, 0.12)
                        textColor: tone
                        borderColor: Qt.rgba(tone.r, tone.g, tone.b, 0.25)
                        padding: 2
                        leftPadding: 8
                        rightPadding: 8
                    }

                    Badge {
                        visible: root.durationMin > 0
                        interactive: false
                        theme: root.theme
                        text: root.theme.formatDuration(root.durationMin)
                        accessibleName: qsTr("Planned duration: %1").arg(text)
                        fillColor: root.theme.chip
                        textColor: root.theme.muted
                        padding: 2
                        leftPadding: 8
                        rightPadding: 8
                    }

                    Badge {
                        visible: root.folderName.length > 0
                        interactive: false
                        theme: root.theme
                        text: root.folderName
                        accessibleName: qsTr("Folder: %1").arg(text)
                        fillColor: root.theme.chip
                        textColor: root.theme.panelText
                        borderColor: root.theme.border
                        padding: 2
                        leftPadding: 8
                        rightPadding: 8
                    }

                    Badge {
                        visible: root.subtaskTotal > 0
                        interactive: true
                        theme: root.theme
                        text: (root.expanded ? "▾ " : "▸ ")
                            + qsTr("%1/%2").arg(root.subtaskDone).arg(root.subtaskTotal)
                        accessibleName: qsTr("%1 of %2 subtasks completed. Click to %3")
                            .arg(root.subtaskDone)
                            .arg(root.subtaskTotal)
                            .arg(root.expanded ? qsTr("collapse") : qsTr("expand"))
                        fillColor: root.expanded
                            ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.10)
                            : root.theme.chip
                        textColor: root.expanded ? root.theme.accent : root.theme.panelText
                        borderColor: root.expanded
                            ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.22)
                            : root.theme.border
                        padding: 2
                        leftPadding: 8
                        rightPadding: 8
                        onClicked: root.toggleExpandRequested(root.taskId)
                    }

                    Repeater {
                        model: root.tags

                        delegate: TagChip {
                            required property var modelData
                            theme: root.theme
                            tagName: modelData.name
                            tagColour: modelData.colour !== undefined ? modelData.colour : ""
                            removable: false
                            verticalPadding: 2
                        }
                    }
                }
            }

            IconButton {
                Layout.alignment: Qt.AlignTop
                theme: root.theme
                iconKind: "add"
                toolTipText: qsTr("Add subtask")
                visible: !root.editing
                opacity: rowHover.hovered || root.isSelected || activeFocus ? 1 : 0.35
                onClicked: root.beginAddSubtask()
            }

            IconButton {
                Layout.alignment: Qt.AlignTop
                theme: root.theme
                iconKind: "edit"
                toolTipText: qsTr("Rename task")
                visible: !root.editing
                opacity: rowHover.hovered || root.isSelected || activeFocus ? 1 : 0.35
                onClicked: root.beginRename()
            }

            IconButton {
                Layout.alignment: Qt.AlignTop
                theme: root.theme
                iconKind: "delete"
                destructive: true
                toolTipText: qsTr("Delete task")
                visible: !root.editing
                opacity: rowHover.hovered || root.isSelected || activeFocus ? 1 : 0.35
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
                                    taskId: modelData.id,
                                    title: modelData.title,
                                    completed: modelData.completed,
                                    priority: modelData.priority !== undefined ? modelData.priority : 0,
                                    durationMin: modelData.durationMin !== undefined ? modelData.durationMin : 0,
                                    dueTime: modelData.dueTime !== undefined ? modelData.dueTime : "",
                                    folderName: modelData.folderName !== undefined ? modelData.folderName : "",
                                    tags: modelData.tags !== undefined ? modelData.tags : [],
                                    subtaskDone: modelData.subtaskDone !== undefined ? modelData.subtaskDone : 0,
                                    subtaskTotal: modelData.subtaskTotal !== undefined ? modelData.subtaskTotal : 0,
                                    subtasks: modelData.subtasks !== undefined ? modelData.subtasks : [],
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
                                function onAddSubtaskRequested(parentId, subtaskTitle) {
                                    root.addSubtaskRequested(parentId, subtaskTitle)
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
                placeholderText: qsTr("New subtask")
                font.family: root.theme.font
                font.pixelSize: root.theme.bodySize - 1
                Accessible.name: qsTr("New subtask")
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
