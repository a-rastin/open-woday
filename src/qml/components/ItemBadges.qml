import QtQuick
import QtQuick.Layouts
import ".."

Flow {
    id: flow

    required property Theme theme
    property string leadingText: ""
    property string leadingAccessibleName: ""
    property bool leadingNeutral: false
    property int priority: 0
    property int durationMin: 0
    property string folderName: ""
    property var tags: []
    property bool showSubtasks: false
    property int subtaskDone: 0
    property int subtaskTotal: 0
    property bool subtaskExpanded: false

    signal subtaskToggleRequested()

    spacing: 6
    visible: leadingText.length > 0
        || priority > 0
        || durationMin > 0
        || folderName.length > 0
        || (showSubtasks && subtaskTotal > 0)
        || (tags && tags.length > 0)

    Badge {
        visible: flow.leadingText.length > 0
        interactive: false
        theme: flow.theme
        text: flow.leadingText
        accessibleName: flow.leadingAccessibleName
        fillColor: flow.leadingNeutral
            ? flow.theme.chip
            : Qt.rgba(flow.theme.accent.r, flow.theme.accent.g, flow.theme.accent.b, 0.10)
        textColor: flow.leadingNeutral ? flow.theme.panelText : flow.theme.accent
        borderColor: flow.leadingNeutral
            ? flow.theme.border
            : Qt.rgba(flow.theme.accent.r, flow.theme.accent.g, flow.theme.accent.b, 0.22)
        padding: 2
        leftPadding: 8
        rightPadding: 8
    }

    Badge {
        readonly property color tone: flow.theme.priorityColor(flow.priority)
        visible: flow.priority > 0
        interactive: false
        theme: flow.theme
        text: flow.theme.priorityLabel(flow.priority)
        accessibleName: qsTr("Priority: %1").arg(text)
        fillColor: Qt.rgba(tone.r, tone.g, tone.b, 0.12)
        textColor: tone
        borderColor: Qt.rgba(tone.r, tone.g, tone.b, 0.25)
        padding: 2
        leftPadding: 8
        rightPadding: 8
    }

    Badge {
        visible: flow.durationMin > 0
        interactive: false
        theme: flow.theme
        text: flow.theme.formatDuration(flow.durationMin)
        accessibleName: qsTr("Planned duration: %1").arg(text)
        fillColor: flow.theme.chip
        textColor: flow.theme.muted
        padding: 2
        leftPadding: 8
        rightPadding: 8
    }

    Badge {
        visible: flow.folderName.length > 0
        interactive: false
        theme: flow.theme
        text: flow.folderName
        accessibleName: qsTr("Folder: %1").arg(text)
        fillColor: flow.theme.chip
        textColor: flow.theme.panelText
        borderColor: flow.theme.border
        padding: 2
        leftPadding: 8
        rightPadding: 8
    }

    Badge {
        visible: flow.showSubtasks && flow.subtaskTotal > 0
        interactive: true
        theme: flow.theme
        text: (flow.subtaskExpanded ? "▾ " : "▸ ")
            + qsTr("%1/%2").arg(flow.subtaskDone).arg(flow.subtaskTotal)
        accessibleName: qsTr("%1 of %2 subtasks completed. Click to %3")
            .arg(flow.subtaskDone)
            .arg(flow.subtaskTotal)
            .arg(flow.subtaskExpanded ? qsTr("collapse") : qsTr("expand"))
        fillColor: flow.subtaskExpanded
            ? Qt.rgba(flow.theme.accent.r, flow.theme.accent.g, flow.theme.accent.b, 0.10)
            : flow.theme.chip
        textColor: flow.subtaskExpanded ? flow.theme.accent : flow.theme.panelText
        borderColor: flow.subtaskExpanded
            ? Qt.rgba(flow.theme.accent.r, flow.theme.accent.g, flow.theme.accent.b, 0.22)
            : flow.theme.border
        padding: 2
        leftPadding: 8
        rightPadding: 8
        onClicked: flow.subtaskToggleRequested()
    }

    Repeater {
        model: flow.tags

        delegate: TagChip {
            required property var modelData
            theme: flow.theme
            tagName: modelData.name
            tagColour: modelData.colour !== undefined ? modelData.colour : ""
            removable: false
            verticalPadding: 2
        }
    }
}
