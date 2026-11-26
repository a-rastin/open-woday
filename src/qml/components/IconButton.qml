import QtQuick
import QtQuick.Controls
import ".."

AbstractButton {
    id: control

    required property Theme theme
    // Supported geometric icons: "close", "edit", "delete", "add". Falls back to glyph text.
    property string iconKind: ""
    property string glyph: ""
    property bool destructive: false
    property string toolTipText: ""

    readonly property color iconColor: destructive && hovered
        ? theme.accent
        : (hovered ? theme.panelText : theme.muted)

    implicitWidth: 28
    implicitHeight: 28
    padding: 4
    Accessible.role: Accessible.Button
    Accessible.name: toolTipText.length > 0 ? toolTipText : (text.length > 0 ? text : glyph)
    ToolTip.visible: hovered && toolTipText.length > 0
    ToolTip.text: toolTipText
    ToolTip.delay: 400

    background: Rectangle {
        radius: control.theme.radiusControl
        color: {
            if (!control.hovered && !control.down)
                return "transparent"
            if (control.destructive)
                return Qt.rgba(control.theme.accent.r, control.theme.accent.g, control.theme.accent.b, 0.12)
            return control.theme.chip
        }
    }

    contentItem: Item {
        implicitWidth: 16
        implicitHeight: 16

        // Close icon ("x")
        Item {
            anchors.centerIn: parent
            width: 12
            height: 12
            visible: control.iconKind === "close"

            Rectangle {
                anchors.centerIn: parent
                width: 12
                height: 1.5
                radius: 1
                rotation: 45
                color: control.iconColor
            }
            Rectangle {
                anchors.centerIn: parent
                width: 12
                height: 1.5
                radius: 1
                rotation: -45
                color: control.iconColor
            }
        }

        // Rename / edit icon (minimal geometric pencil stroke)
        Item {
            anchors.centerIn: parent
            width: 12
            height: 12
            visible: control.iconKind === "edit"

            Rectangle {
                anchors.centerIn: parent
                width: 11
                height: 3
                radius: 1
                rotation: -45
                color: "transparent"
                border.color: control.iconColor
                border.width: 1.2
            }
            Rectangle {
                x: 1
                y: 9
                width: 3
                height: 2
                color: control.iconColor
            }
        }

        // Soft-delete icon (minimal geometric bin)
        Item {
            anchors.centerIn: parent
            width: 12
            height: 13
            visible: control.iconKind === "delete"

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 0
                width: 4
                height: 1.5
                color: control.iconColor
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 2
                width: 12
                height: 1.5
                radius: 0.75
                color: control.iconColor
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 4
                width: 9
                height: 9
                radius: 1.5
                color: "transparent"
                border.color: control.iconColor
                border.width: 1.4
            }
        }

        // Add icon ("+")
        Item {
            anchors.centerIn: parent
            width: 12
            height: 12
            visible: control.iconKind === "add"

            Rectangle {
                anchors.centerIn: parent
                width: 11
                height: 1.5
                radius: 0.75
                color: control.iconColor
            }
            Rectangle {
                anchors.centerIn: parent
                width: 1.5
                height: 11
                radius: 0.75
                color: control.iconColor
            }
        }

        Text {
            anchors.centerIn: parent
            visible: control.iconKind.length === 0
            text: control.glyph.length > 0 ? control.glyph : control.text
            color: control.iconColor
            font.family: control.theme.font
            font.pixelSize: control.theme.bodySize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
