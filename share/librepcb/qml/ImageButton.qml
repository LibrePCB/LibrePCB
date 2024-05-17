import QtQuick 6.2
import QtQuick.Controls 6.2

Item {
    id:root
    width: 30
    height: width

    property alias icon: image.source
    signal clicked()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    Image {
        id: image
        anchors.centerIn: parent
        width: mouseArea.containsMouse ? parent.width * 1.2 : parent.width
        height: mouseArea.containsMouse ? parent.height * 1.2 : parent.height
    }
}
