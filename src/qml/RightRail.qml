import QtQuick

// Hosts the Task detail pane when a Task is selected; otherwise remains a
// clean idle rail ready for the Inbox surface in a later phase.
Rectangle {
    id: rightRail

    required property Theme theme

    implicitWidth: 7 * theme.titleSize
    width: implicitWidth

    color: rightRail.theme.panel
    radius: rightRail.theme.radiusPanel

    Loader {
        anchors.fill: parent
        anchors.margins: rightRail.theme.gap * 1.25
        active: itemModel.hasSelection
        sourceComponent: Component {
            TaskDetailPane {
                theme: rightRail.theme
            }
        }
    }
}
