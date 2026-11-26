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
        anchors.leftMargin: 6
        anchors.rightMargin: 4
        spacing: root.theme.gap * 0.75

        Checkbox {
            theme: root.theme
            checked: root.completed
            priority: root.priority
            accessibleName: qsTr("Complete %1").arg(root.title)
            onCheckToggled: root.toggleCompletedRequested(root.taskId, !root.completed)
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: Math.max(titleLabel.implicitHeight, renameField.implicitHeight)

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

        IconButton {
            theme: root.theme
            iconKind: "edit"
            toolTipText: qsTr("Rename task")
            visible: !root.editing
            opacity: rowHover.hovered || root.selected || activeFocus ? 1 : 0.35
            onClicked: root.beginRename()
        }

        IconButton {
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
