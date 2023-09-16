import QtQuick 2.0
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.2

Column {
    Rectangle {
        width: parent.width
        height: childrenRect.height + 6
        color: "transparent"
        border.color: "gray"
        border.width: 1
        Label {
            text: "Project: Demo"
            color: "white"
            elide: Label.ElideRight
            x: 3
            y: 3
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }
}
