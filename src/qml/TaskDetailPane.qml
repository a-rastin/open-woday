import QtQuick
import QtQuick.Controls
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

    function closePopups() {
        addDetailPopup.close()
        dueDatePopup.close()
        priorityPopup.close()
        durationPopup.close()
        folderPopup.close()
        tagPopup.close()
    }

    function positionPopupBelow(popup, anchorItem) {
        const pos = anchorItem.mapToItem(pane, 0, anchorItem.height + 6)
        const maxLeft = pane.width - popup.width
        popup.x = popup.width <= pane.width ? Math.max(0, Math.min(pos.x, maxLeft)) : maxLeft
        popup.y = pos.y
    }

    function openDueDateEditor(anchorItem) {
        closePopups()
        dueDateInput.text = itemModel.hasDueDate
            ? itemModel.dueDate
            : pane.theme.isoDateOffset(0)
        dueTimeInput.text = itemModel.hasDueTime ? itemModel.dueTime : ""
        positionPopupBelow(dueDatePopup, anchorItem)
        dueDatePopup.open()
        dueDateInput.forceActiveFocus()
        dueDateInput.selectAll()
    }

    function submitDueDateInput() {
        const dateText = dueDateInput.text.trim()
        const timeText = dueTimeInput.text.trim()
        if (itemModel.setTaskDueDateTime(itemModel.selectedId, dateText, timeText))
            dueDatePopup.close()
    }

    function openPriorityEditor(anchorItem) {
        closePopups()
        positionPopupBelow(priorityPopup, anchorItem)
        priorityPopup.open()
    }

    function openDurationEditor(anchorItem) {
        closePopups()
        durationInput.text = itemModel.hasDuration ? String(itemModel.durationMin) : ""
        positionPopupBelow(durationPopup, anchorItem)
        durationPopup.open()
        durationInput.forceActiveFocus()
        durationInput.selectAll()
    }

    function submitDurationInput() {
        const trimmed = durationInput.text.trim()
        const mins = Number(trimmed)
        if (!Number.isInteger(mins) || mins <= 0) {
            itemModel.setTaskDuration(itemModel.selectedId, 0)
            return
        }
        if (itemModel.setTaskDuration(itemModel.selectedId, mins))
            durationPopup.close()
    }

    function openFolderEditor(anchorItem) {
        closePopups()
        newFolderInput.text = ""
        positionPopupBelow(folderPopup, anchorItem)
        folderPopup.open()
    }

    function submitNewFolderInput() {
        const trimmed = newFolderInput.text.trim()
        if (itemModel.createAndAssignFolder(itemModel.selectedId, trimmed)) {
            newFolderInput.text = ""
            folderPopup.close()
        }
    }

    function openTagEditor(anchorItem) {
        closePopups()
        tagInput.text = ""
        positionPopupBelow(tagPopup, anchorItem)
        tagPopup.open()
        tagInput.forceActiveFocus()
    }

    function submitNewTagInput() {
        if (itemModel.attachTagByName(itemModel.selectedId, tagInput.text))
            tagInput.text = ""
    }

    function isTagAttached(tagId) {
        const attached = itemModel.tags
        for (let i = 0; i < attached.length; ++i) {
            if (attached[i].id === tagId)
                return true
        }
        return false
    }

    Component.onCompleted: syncTitleFromModel()

    Connections {
        target: itemModel
        function onSelectionChanged() {
            pane.closePopups()
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
                    pane.closePopups()
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
                id: dueDateBadge
                visible: itemModel.hasDueDate
                theme: pane.theme
                text: pane.theme.formatDueDateTime(itemModel.dueDate, itemModel.dueTime)
                accessibleName: qsTr("Due date: %1. Click to edit or clear").arg(text)
                fillColor: itemModel.hasDueTime
                    ? Qt.rgba(pane.theme.accent.r, pane.theme.accent.g, pane.theme.accent.b, 0.10)
                    : pane.theme.chip
                textColor: itemModel.hasDueTime ? pane.theme.accent : pane.theme.panelText
                borderColor: itemModel.hasDueTime
                    ? Qt.rgba(pane.theme.accent.r, pane.theme.accent.g, pane.theme.accent.b, 0.22)
                    : "transparent"
                onClicked: pane.openDueDateEditor(dueDateBadge)
            }

            Badge {
                id: priorityBadge
                readonly property color tone: pane.theme.priorityColor(itemModel.priority)
                visible: itemModel.priority > 0
                theme: pane.theme
                text: pane.theme.priorityLabel(itemModel.priority)
                accessibleName: qsTr("Priority: %1. Click to edit or clear").arg(text)
                fillColor: Qt.rgba(tone.r, tone.g, tone.b, 0.12)
                textColor: tone
                borderColor: Qt.rgba(tone.r, tone.g, tone.b, 0.25)
                onClicked: pane.openPriorityEditor(priorityBadge)
            }

            Badge {
                id: durationBadge
                visible: itemModel.hasDuration
                theme: pane.theme
                text: pane.theme.formatDuration(itemModel.durationMin)
                accessibleName: qsTr("Planned duration: %1. Click to edit or clear").arg(text)
                onClicked: pane.openDurationEditor(durationBadge)
            }

            Badge {
                id: folderBadge
                visible: itemModel.hasFolder
                theme: pane.theme
                text: itemModel.folderName
                borderColor: pane.theme.border
                accessibleName: qsTr("Folder: %1. Click to change or clear").arg(text)
                onClicked: pane.openFolderEditor(folderBadge)
            }

            Repeater {
                model: itemModel.tags

                delegate: TagChip {
                    required property var modelData
                    theme: pane.theme
                    tagName: modelData.name
                    tagColour: modelData.colour !== undefined ? modelData.colour : ""
                    removable: true
                    onRemoveRequested: itemModel.detachTag(itemModel.selectedId, modelData.id)
                }
            }

            Badge {
                id: addDetailBadge
                theme: pane.theme
                text: "+"
                accessibleName: qsTr("Add task detail")
                onClicked: {
                    pane.addDetailRequested()
                    pane.closePopups()
                    pane.positionPopupBelow(addDetailPopup, addDetailBadge)
                    addDetailPopup.open()
                }
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

    Popup {
        id: addDetailPopup
        width: Math.max(148, Math.min(180, pane.width))
        padding: 6
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: pane.theme.panel
            border.color: pane.theme.border
            border.width: 1
            radius: pane.theme.radiusControl
        }

        contentItem: ColumnLayout {
            width: addDetailPopup.availableWidth
            spacing: 2

            AbstractButton {
                Layout.fillWidth: true
                visible: !itemModel.hasDueDate
                padding: 6
                leftPadding: 8
                rightPadding: 8
                Accessible.role: Accessible.MenuItem
                Accessible.name: qsTr("Add due date")
                onClicked: pane.openDueDateEditor(addDetailBadge)

                background: Rectangle {
                    radius: pane.theme.radiusControl - 2
                    color: parent.hovered ? pane.theme.chip : "transparent"
                }

                contentItem: Text {
                    text: qsTr("Due date")
                    color: pane.theme.panelText
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    verticalAlignment: Text.AlignVCenter
                }
            }

            AbstractButton {
                Layout.fillWidth: true
                visible: itemModel.priority === 0
                padding: 6
                leftPadding: 8
                rightPadding: 8
                Accessible.role: Accessible.MenuItem
                Accessible.name: qsTr("Add priority")
                onClicked: pane.openPriorityEditor(addDetailBadge)

                background: Rectangle {
                    radius: pane.theme.radiusControl - 2
                    color: parent.hovered ? pane.theme.chip : "transparent"
                }

                contentItem: Text {
                    text: qsTr("Priority")
                    color: pane.theme.panelText
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    verticalAlignment: Text.AlignVCenter
                }
            }

            AbstractButton {
                Layout.fillWidth: true
                visible: !itemModel.hasDuration
                padding: 6
                leftPadding: 8
                rightPadding: 8
                Accessible.role: Accessible.MenuItem
                Accessible.name: qsTr("Add duration")
                onClicked: pane.openDurationEditor(addDetailBadge)

                background: Rectangle {
                    radius: pane.theme.radiusControl - 2
                    color: parent.hovered ? pane.theme.chip : "transparent"
                }

                contentItem: Text {
                    text: qsTr("Duration")
                    color: pane.theme.panelText
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    verticalAlignment: Text.AlignVCenter
                }
            }

            AbstractButton {
                Layout.fillWidth: true
                visible: !itemModel.hasFolder
                padding: 6
                leftPadding: 8
                rightPadding: 8
                Accessible.role: Accessible.MenuItem
                Accessible.name: qsTr("Add folder")
                onClicked: pane.openFolderEditor(addDetailBadge)

                background: Rectangle {
                    radius: pane.theme.radiusControl - 2
                    color: parent.hovered ? pane.theme.chip : "transparent"
                }

                contentItem: Text {
                    text: qsTr("Folder")
                    color: pane.theme.panelText
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    verticalAlignment: Text.AlignVCenter
                }
            }

            AbstractButton {
                Layout.fillWidth: true
                padding: 6
                leftPadding: 8
                rightPadding: 8
                Accessible.role: Accessible.MenuItem
                Accessible.name: qsTr("Add tag")
                onClicked: pane.openTagEditor(addDetailBadge)

                background: Rectangle {
                    radius: pane.theme.radiusControl - 2
                    color: parent.hovered ? pane.theme.chip : "transparent"
                }

                contentItem: Text {
                    text: qsTr("Add tag")
                    color: pane.theme.panelText
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Popup {
        id: dueDatePopup
        width: 228
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: pane.theme.panel
            border.color: pane.theme.border
            border.width: 1
            radius: pane.theme.radiusControl
        }

        contentItem: ColumnLayout {
            width: dueDatePopup.availableWidth
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Due date")
                    color: pane.theme.muted
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 2
                }

                Badge {
                    visible: itemModel.hasDueDate
                    theme: pane.theme
                    text: qsTr("Clear")
                    textColor: pane.theme.accent
                    accessibleName: qsTr("Clear due date")
                    padding: 2
                    leftPadding: 8
                    rightPadding: 8
                    onClicked: {
                        if (itemModel.clearTaskDueDate(itemModel.selectedId))
                            dueDatePopup.close()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: [
                        { label: qsTr("Today"), offset: 0 },
                        { label: qsTr("Tomorrow"), offset: 1 }
                    ]

                    delegate: Badge {
                        required property var modelData
                        readonly property string targetDate: pane.theme.isoDateOffset(modelData.offset)
                        readonly property bool isCurrent: itemModel.hasDueDate && itemModel.dueDate === targetDate
                        theme: pane.theme
                        text: modelData.label
                        fillColor: isCurrent ? pane.theme.accent : pane.theme.chip
                        textColor: isCurrent ? pane.theme.panel : pane.theme.panelText
                        padding: 3
                        leftPadding: 8
                        rightPadding: 8
                        onClicked: {
                            dueDateInput.text = targetDate
                            itemModel.setTaskDueDateTime(
                                itemModel.selectedId,
                                targetDate,
                                dueTimeInput.text.trim())
                        }
                    }
                }
            }

            TextField {
                id: dueDateInput
                Layout.fillWidth: true
                placeholderText: qsTr("YYYY-MM-DD")
                font.family: pane.theme.font
                font.pixelSize: pane.theme.bodySize - 1
                Accessible.name: qsTr("Due date (YYYY-MM-DD)")
                onAccepted: pane.submitDueDateInput()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Time (24h)")
                    color: pane.theme.muted
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 2
                }

                Badge {
                    visible: itemModel.hasDueTime || dueTimeInput.text.trim().length > 0
                    theme: pane.theme
                    text: qsTr("Clear time")
                    textColor: pane.theme.accent
                    accessibleName: qsTr("Clear due time")
                    padding: 2
                    leftPadding: 8
                    rightPadding: 8
                    onClicked: {
                        dueTimeInput.text = ""
                        if (itemModel.hasDueTime)
                            itemModel.clearTaskDueTime(itemModel.selectedId)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                TextField {
                    id: dueTimeInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("HH:mm")
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    Accessible.name: qsTr("Due time (24-hour HH:mm)")
                    onAccepted: pane.submitDueDateInput()
                }

                Button {
                    text: qsTr("Set")
                    onClicked: pane.submitDueDateInput()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: ["09:00", "12:00", "14:00", "17:00"]

                    delegate: Badge {
                        required property string modelData
                        readonly property bool isCurrent: itemModel.hasDueTime && itemModel.dueTime === modelData
                        theme: pane.theme
                        text: modelData
                        fillColor: isCurrent ? pane.theme.accent : pane.theme.chip
                        textColor: isCurrent ? pane.theme.panel : pane.theme.panelText
                        padding: 3
                        leftPadding: 8
                        rightPadding: 8
                        onClicked: {
                            dueTimeInput.text = modelData
                            itemModel.setTaskDueDateTime(
                                itemModel.selectedId,
                                dueDateInput.text.trim(),
                                modelData)
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: priorityPopup
        width: Math.max(152, Math.min(180, pane.width))
        padding: 6
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: pane.theme.panel
            border.color: pane.theme.border
            border.width: 1
            radius: pane.theme.radiusControl
        }

        contentItem: ColumnLayout {
            width: priorityPopup.availableWidth
            spacing: 2

            Text {
                Layout.fillWidth: true
                leftPadding: 8
                topPadding: 2
                bottomPadding: 2
                text: qsTr("Priority")
                color: pane.theme.muted
                font.family: pane.theme.font
                font.pixelSize: pane.theme.bodySize - 2
            }

            Repeater {
                model: [3, 2, 1, 0]

                delegate: AbstractButton {
                    id: priorityOption
                    required property int modelData
                    readonly property bool isCurrent: itemModel.priority === modelData
                    readonly property color levelColor: pane.theme.priorityColor(modelData)

                    Layout.fillWidth: true
                    padding: 6
                    leftPadding: 8
                    rightPadding: 8
                    Accessible.role: Accessible.MenuItem
                    Accessible.name: pane.theme.priorityLabel(modelData)
                    onClicked: {
                        if (itemModel.setTaskPriority(itemModel.selectedId, modelData))
                            priorityPopup.close()
                    }

                    background: Rectangle {
                        radius: pane.theme.radiusControl - 2
                        color: priorityOption.isCurrent || priorityOption.hovered
                            ? pane.theme.chip
                            : "transparent"
                    }

                    contentItem: RowLayout {
                        spacing: 8

                        Rectangle {
                            implicitWidth: 8
                            implicitHeight: 8
                            radius: 4
                            color: priorityOption.modelData > 0
                                ? priorityOption.levelColor
                                : "transparent"
                            border.color: priorityOption.levelColor
                            border.width: priorityOption.modelData > 0 ? 0 : 1.5
                        }

                        Text {
                            Layout.fillWidth: true
                            text: pane.theme.priorityLabel(priorityOption.modelData)
                            color: pane.theme.panelText
                            font.family: pane.theme.font
                            font.pixelSize: pane.theme.bodySize - 1
                            font.bold: priorityOption.isCurrent
                        }

                        Text {
                            visible: priorityOption.isCurrent
                            text: "✓"
                            color: pane.theme.muted
                            font.family: pane.theme.font
                            font.pixelSize: pane.theme.bodySize - 2
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: durationPopup
        width: 216
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: pane.theme.panel
            border.color: pane.theme.border
            border.width: 1
            radius: pane.theme.radiusControl
        }

        contentItem: ColumnLayout {
            width: durationPopup.availableWidth
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Planned duration")
                    color: pane.theme.muted
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 2
                }

                Badge {
                    visible: itemModel.hasDuration
                    theme: pane.theme
                    text: qsTr("Clear")
                    textColor: pane.theme.accent
                    accessibleName: qsTr("Clear planned duration")
                    padding: 2
                    leftPadding: 8
                    rightPadding: 8
                    onClicked: {
                        if (itemModel.clearTaskDuration(itemModel.selectedId))
                            durationPopup.close()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                TextField {
                    id: durationInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Minutes")
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 1
                        top: 10080
                    }
                    Accessible.name: qsTr("Planned duration in minutes")
                    onAccepted: pane.submitDurationInput()
                }

                Button {
                    text: qsTr("Set")
                    onClicked: pane.submitDurationInput()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: [15, 30, 45, 60, 90, 120]

                    delegate: Badge {
                        required property int modelData
                        readonly property bool isCurrent: itemModel.durationMin === modelData
                        theme: pane.theme
                        text: pane.theme.formatDuration(modelData)
                        fillColor: isCurrent ? pane.theme.accent : pane.theme.chip
                        textColor: isCurrent ? pane.theme.panel : pane.theme.panelText
                        padding: 3
                        leftPadding: 8
                        rightPadding: 8
                        onClicked: {
                            if (itemModel.setTaskDuration(itemModel.selectedId, modelData))
                                durationPopup.close()
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: folderPopup
        width: 220
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: pane.theme.panel
            border.color: pane.theme.border
            border.width: 1
            radius: pane.theme.radiusControl
        }

        contentItem: ColumnLayout {
            width: folderPopup.availableWidth
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Folder")
                    color: pane.theme.muted
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 2
                }

                Badge {
                    visible: itemModel.hasFolder
                    theme: pane.theme
                    text: qsTr("Clear")
                    textColor: pane.theme.accent
                    accessibleName: qsTr("Clear folder")
                    padding: 2
                    leftPadding: 8
                    rightPadding: 8
                    onClicked: {
                        if (itemModel.clearTaskFolder(itemModel.selectedId))
                            folderPopup.close()
                    }
                }
            }

            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(160, folderListColumn.implicitHeight)
                visible: itemModel.folders.length > 0
                contentWidth: width
                contentHeight: folderListColumn.implicitHeight
                clip: true

                ColumnLayout {
                    id: folderListColumn
                    width: parent.width
                    spacing: 2

                    Repeater {
                        model: itemModel.folders

                        delegate: AbstractButton {
                            id: folderOption
                            required property var modelData
                            readonly property bool isCurrent: itemModel.folderId === modelData.id

                            Layout.fillWidth: true
                            padding: 6
                            leftPadding: 8
                            rightPadding: 8
                            Accessible.role: Accessible.MenuItem
                            Accessible.name: modelData.name
                            onClicked: {
                                if (itemModel.setTaskFolder(itemModel.selectedId, modelData.id))
                                    folderPopup.close()
                            }

                            background: Rectangle {
                                radius: pane.theme.radiusControl - 2
                                color: folderOption.isCurrent || folderOption.hovered
                                    ? pane.theme.chip
                                    : "transparent"
                            }

                            contentItem: RowLayout {
                                spacing: 8

                                Text {
                                    Layout.fillWidth: true
                                    text: folderOption.modelData.name
                                    color: pane.theme.panelText
                                    font.family: pane.theme.font
                                    font.pixelSize: pane.theme.bodySize - 1
                                    font.bold: folderOption.isCurrent
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: folderOption.isCurrent
                                    text: "✓"
                                    color: pane.theme.muted
                                    font.family: pane.theme.font
                                    font.pixelSize: pane.theme.bodySize - 2
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                TextField {
                    id: newFolderInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("New folder")
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    Accessible.name: qsTr("New folder name")
                    onAccepted: pane.submitNewFolderInput()
                }

                Button {
                    text: qsTr("Create")
                    enabled: newFolderInput.text.trim().length > 0
                    onClicked: pane.submitNewFolderInput()
                }
            }
        }
    }

    Popup {
        id: tagPopup
        width: 228
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: pane.theme.panel
            border.color: pane.theme.border
            border.width: 1
            radius: pane.theme.radiusControl
        }

        contentItem: ColumnLayout {
            width: tagPopup.availableWidth
            spacing: 6

            Text {
                Layout.fillWidth: true
                text: qsTr("Tags")
                color: pane.theme.muted
                font.family: pane.theme.font
                font.pixelSize: pane.theme.bodySize - 2
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                TextField {
                    id: tagInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("New or existing tag")
                    font.family: pane.theme.font
                    font.pixelSize: pane.theme.bodySize - 1
                    Accessible.name: qsTr("Tag name")
                    onAccepted: pane.submitNewTagInput()
                }

                Button {
                    text: qsTr("Add")
                    enabled: tagInput.text.trim().length > 0
                    onClicked: pane.submitNewTagInput()
                }
            }

            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(180, tagListColumn.implicitHeight)
                visible: itemModel.availableTags.length > 0
                contentWidth: width
                contentHeight: tagListColumn.implicitHeight
                clip: true

                ColumnLayout {
                    id: tagListColumn
                    width: parent.width
                    spacing: 2

                    Repeater {
                        model: itemModel.availableTags

                        delegate: AbstractButton {
                            id: tagOption
                            required property var modelData
                            readonly property string tagColour: modelData.colour !== undefined ? modelData.colour : ""
                            readonly property bool attached: pane.isTagAttached(modelData.id)

                            Layout.fillWidth: true
                            padding: 6
                            leftPadding: 8
                            rightPadding: 8
                            Accessible.role: Accessible.MenuItem
                            Accessible.name: modelData.name
                            onClicked: {
                                if (tagOption.attached)
                                    itemModel.detachTag(itemModel.selectedId, tagOption.modelData.id)
                                else
                                    itemModel.attachTag(itemModel.selectedId, tagOption.modelData.id)
                            }

                            background: Rectangle {
                                radius: pane.theme.radiusControl - 2
                                color: tagOption.attached || tagOption.hovered
                                    ? pane.theme.chip
                                    : "transparent"
                            }

                            contentItem: RowLayout {
                                spacing: 8

                                Rectangle {
                                    implicitWidth: 10
                                    implicitHeight: 10
                                    radius: 5
                                    color: pane.theme.tagFillColor(tagOption.modelData.name, tagOption.tagColour)
                                    border.color: pane.theme.tagBorderColor(tagOption.modelData.name, tagOption.tagColour)
                                    border.width: 1
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: tagOption.modelData.name
                                    color: pane.theme.panelText
                                    font.family: pane.theme.font
                                    font.pixelSize: pane.theme.bodySize - 1
                                    font.bold: tagOption.attached
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: tagOption.attached
                                    text: "✓"
                                    color: pane.theme.muted
                                    font.family: pane.theme.font
                                    font.pixelSize: pane.theme.bodySize - 2
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
