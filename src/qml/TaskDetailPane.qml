import QtQuick
import QtQuick.Layouts
import "components"

Item {
    id: pane

    required property Theme theme

    signal addDetailRequested()

    function syncTitleFromModel() {
        if (!titleInput.activeFocus)
            titleInput.text = itemModel.title
    }

    function commitTitle() {
        const trimmed = titleInput.text.trim()
        if (trimmed.length === 0) {
            titleInput.text = itemModel.title
            return
        }
        if (trimmed !== itemModel.title) {
            if (!itemModel.renameTask(itemModel.selectedId, trimmed))
                titleInput.text = itemModel.title
        }
    }

    Component.onCompleted: syncTitleFromModel()

    Connections {
        target: itemModel
        function onSelectionChanged() {
            titleInput.focus = false
            titleInput.text = itemModel.title
        }
        function onSelectedTaskChanged() {
            pane.syncTitleFromModel()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: pane.theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: pane.theme.gap / 2

            Text {
                Layout.fillWidth: true
                text: qsTr("Task")
                color: pane.theme.muted
                font.family: pane.theme.font
                font.pixelSize: pane.theme.bodySize - 1
                elide: Text.ElideRight
            }

            IconButton {
                theme: pane.theme
                iconKind: "delete"
                destructive: true
                toolTipText: qsTr("Delete task")
                onClicked: itemModel.deleteTask(itemModel.selectedId)
            }

            IconButton {
                theme: pane.theme
                iconKind: "close"
                toolTipText: qsTr("Close task details")
                onClicked: {
                    pane.commitTitle()
                    itemModel.clearSelection()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: pane.theme.gap * 0.75

            Checkbox {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 3
                theme: pane.theme
                checked: itemModel.completed
                priority: itemModel.priority
                accessibleName: qsTr("Complete %1").arg(itemModel.title)
                onCheckToggled: itemModel.setTaskCompleted(itemModel.selectedId, !itemModel.completed)
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: Math.max(28, titleInput.implicitHeight + 8)
                radius: pane.theme.radiusControl
                color: titleInput.activeFocus ? pane.theme.chip : "transparent"
                border.color: titleInput.activeFocus ? pane.theme.border : "transparent"
                border.width: titleInput.activeFocus ? 1 : 0

                TextInput {
                    id: titleInput
                    anchors.fill: parent
                    anchors.margins: 4
                    text: itemModel.title
                    color: itemModel.completed ? pane.theme.muted : pane.theme.panelText
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize + 2
                    font.strikeout: itemModel.completed
                    wrapMode: TextInput.Wrap
                    selectByMouse: true
                    Accessible.role: Accessible.EditableText
                    Accessible.name: qsTr("Task title")

                    Keys.onReturnPressed: (event) => {
                        event.accepted = true
                        pane.commitTitle()
                        titleInput.focus = false
                    }
                    Keys.onEscapePressed: (event) => {
                        event.accepted = true
                        titleInput.text = itemModel.title
                        titleInput.focus = false
                    }
                    onActiveFocusChanged: {
                        if (!activeFocus)
                            pane.commitTitle()
                    }
                }
            }
        }

        // Set-only badge row: displays only attributes set on the Task plus
        // the "+" trigger for unset details; no empty placeholder chips appear.
        Flow {
            id: badgeRow
            Layout.fillWidth: true
            spacing: pane.theme.gap / 2

            Badge {
                id: addDetailBadge
                theme: pane.theme
                text: "+"
                accessibleName: qsTr("Add task detail")
                onClicked: pane.addDetailRequested()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: itemModel.error
            color: pane.theme.accent
            font.family: pane.theme.font
            font.pixelSize: pane.theme.bodySize - 1
            wrapMode: Text.Wrap
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
