import QtQuick 2.0
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.2

ColumnLayout {
    Rectangle {
        Layout.fillWidth: true
        height: childrenRect.height + 6
        color: "transparent"
        border.color: "gray"
        border.width: 1
        Label {
            text: cppWindow.currentProject ? ("Project: " + cppWindow.currentProject.name) : "No project opened"
            color: "white"
            elide: Label.ElideRight
            x: 3
            y: 3
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }
    ListView {
        id: lstSchematics
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: cppWindow.currentProject ? cppWindow.currentProject.schematics : null
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
                    color: lstSchematicsMouseArea.hasHover ? "darkgrey" : "black"
                }
            }
            MouseArea {
                id: lstSchematicsMouseArea
                anchors.fill: parent
                hoverEnabled: true
                property bool hasHover: false
                onEntered: hasHover = true
                onExited: hasHover = false
                onClicked: lstSchematics.currentIndex = index
            }
        }
        highlight: Rectangle {
            color: "grey"
        }
        focus: true
    }
}
