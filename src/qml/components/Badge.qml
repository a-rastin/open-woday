import QtQuick
import QtQuick.Controls
import ".."

AbstractButton {
    id: control

    required property Theme theme
    property color fillColor: theme.chip
    property color textColor: theme.panelText
    property color borderColor: "transparent"
    property bool interactive: true
    property string accessibleName: text

    enabled: interactive
    padding: 4
    leftPadding: 10
    rightPadding: 10
    Accessible.role: Accessible.Button
    Accessible.name: accessibleName

    background: Rectangle {
        radius: control.theme.radiusChip
        color: control.interactive && control.hovered
            ? Qt.darker(control.fillColor, 1.05)
            : control.fillColor
        border.color: control.borderColor
        border.width: control.borderColor.a === 0 ? 0 : 1
    }

    contentItem: Text {
        text: control.text
        color: control.textColor
        font.family: control.theme.font
        font.pixelSize: control.theme.bodySize - 1
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
