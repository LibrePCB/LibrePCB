import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2

ColumnLayout {
    Rectangle {
        Layout.fillWidth: true
        height: childrenRect.height + 6
        color: "transparent"
        border.color: "gray"
        border.width: 1
        Label {
            text: "Local Libraries"
            color: "white"
            elide: Label.ElideRight
            x: 3
            y: 3
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }
    ListView {
        id: lstWorkspaceLibraries
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: cppApp.workspaceLibraries
        delegate: Item {
            width: parent.width
            height: 25
            RowLayout {
                Image {
                    Layout.margins: 2
                    Layout.preferredHeight: 21
                    Layout.preferredWidth: 21
                    source: "qrc:///img/actions/schematic.png"
                    fillMode: Image.PreserveAspectFit
                }
                Text {
                    Layout.margins: 2
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: item.name
                    color: lstWorkspaceLibrariesMouseArea.hasHover ? "darkgrey" : "black"
                }
            }
            MouseArea {
                id: lstWorkspaceLibrariesMouseArea
                anchors.fill: parent
                hoverEnabled: true
                property bool hasHover: false
                onEntered: hasHover = true
                onExited: hasHover = false
                onClicked: lstWorkspaceLibraries.currentIndex = index
            }
        }
        highlight: Rectangle {
            color: "grey"
        }
        focus: true
    }
}
