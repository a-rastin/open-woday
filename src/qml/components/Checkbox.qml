import QtQuick
import QtQuick.Controls
import ".."

CheckBox {
    id: control

    required property Theme theme
    property int priority: 0
    property string accessibleName: qsTr("Toggle task completion")

    signal checkToggled()

    readonly property color priorityColor: {
        if (priority === 3)
            return theme.priorityHigh
        if (priority === 2)
            return theme.priorityMedium
        if (priority === 1)
            return theme.priorityLow
        return theme.muted
    }

    implicitWidth: 20
    implicitHeight: 20
    padding: 0
    Accessible.name: accessibleName
    // The stored Item controls the check state, including when a write fails;
    // clicking must not break the checked binding.
    nextCheckState: function() { return checkState }
    onClicked: control.checkToggled()

    indicator: Rectangle {
        anchors.centerIn: parent
        width: 18
        height: 18
        radius: control.theme.radiusControl / 2
        color: control.checked
            ? (control.priority > 0 ? control.priorityColor : control.theme.accent)
            : control.theme.panel
        border.color: control.checked
            ? (control.priority > 0 ? control.priorityColor : control.theme.accent)
            : control.priorityColor
        border.width: control.priority > 0 ? 2 : 1.5

        Text {
            anchors.centerIn: parent
            text: control.checked ? "✓" : ""
            color: control.theme.panel
            font.family: control.theme.font
            font.pixelSize: control.theme.bodySize - 2
        }
    }
}
