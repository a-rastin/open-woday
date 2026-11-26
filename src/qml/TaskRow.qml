import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Rectangle {
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
    property bool selected: false
    property bool editing: false

    signal selectRequested(string taskId)
    signal toggleCompletedRequested(string taskId, bool completed)
    signal renameRequested(string taskId, string newTitle)
    signal deleteRequested(string taskId)

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

    implicitHeight: Math.max(38, contentRow.implicitHeight + 8)
    radius: theme.radiusControl
    color: root.selected
        ? theme.chip
        : (rowHover.hovered ? Qt.rgba(0, 0, 0, 0.03) : "transparent")
    border.color: root.selected ? theme.border : "transparent"
    border.width: root.selected ? 1 : 0

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
                    font.pixelSize: root.theme.bodySize
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
                    font.pixelSize: root.theme.bodySize
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
            iconKind: "edit"
            toolTipText: qsTr("Rename task")
            visible: !root.editing
            opacity: rowHover.hovered || root.selected || activeFocus ? 1 : 0.35
            onClicked: root.beginRename()
        }

        IconButton {
            Layout.alignment: Qt.AlignTop
            theme: root.theme
            iconKind: "delete"
            destructive: true
            toolTipText: qsTr("Delete task")
            visible: !root.editing
            opacity: rowHover.hovered || root.selected || activeFocus ? 1 : 0.35
            onClicked: root.deleteRequested(root.taskId)
        }
    }
}
