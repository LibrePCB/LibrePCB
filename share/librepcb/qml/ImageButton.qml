import QtQuick 2.0
import QtQuick.Controls 1.5

Item {
    id:root
    width: 30
    height: width

    property alias icon: image.source

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
    }

    Image {
        id: image
        anchors.centerIn: parent
        width: mouseArea.containsMouse ? parent.width * 1.2 : parent.width
        height: mouseArea.containsMouse ? parent.height * 1.2 : parent.height
    }
}
