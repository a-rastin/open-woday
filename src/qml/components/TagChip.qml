import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: chip

    required property Theme theme
    required property string tagName
    property string tagColour: ""
    property bool removable: false
    property real verticalPadding: 3
    property real horizontalPadding: 8

    signal removeRequested()

    readonly property color fillColor: theme.tagFillColor(tagName, tagColour)
    readonly property color textColor: theme.tagTextColor(tagName, tagColour)
    readonly property color borderColor: theme.tagBorderColor(tagName, tagColour)

    implicitWidth: contentRow.implicitWidth + horizontalPadding * 2
    implicitHeight: contentRow.implicitHeight + verticalPadding * 2
    radius: theme.radiusChip
    color: fillColor
    border.color: borderColor
    border.width: 1

    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("Tag: %1").arg(tagName)

    RowLayout {
        id: contentRow
        anchors.centerIn: parent
        spacing: 4

        Text {
            text: chip.tagName
            color: chip.textColor
            font.family: chip.theme.font
            font.pixelSize: chip.theme.bodySize - 1
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        AbstractButton {
            visible: chip.removable
            implicitWidth: 16
            implicitHeight: 16
            padding: 0
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Remove tag %1").arg(chip.tagName)
            onClicked: chip.removeRequested()

            background: Rectangle {
                radius: 8
                color: parent.hovered
                    ? Qt.rgba(chip.textColor.r, chip.textColor.g, chip.textColor.b, 0.14)
                    : "transparent"
            }

            contentItem: Text {
                text: "×"
                color: chip.textColor
                font.family: chip.theme.font
                font.pixelSize: chip.theme.bodySize - 1
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
