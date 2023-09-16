import QtQuick 2.0
import QtQuick.Controls 2.0

AbstractButton {
    id: control
    implicitWidth: parent.width
    implicitHeight: parent.width
    autoExclusive: true

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        property bool hasHover: false
        onEntered: hasHover = true
        onExited: hasHover = false
        onClicked: control.checked = true
    }

    background: Rectangle {
        opacity: enabled ? 1 : 0.3
        color: control.checked ? "#1c1c1c" : (mouseArea.hasHover ? "#424242" : "#353535")
        Rectangle {
            width: 3
            height: control.height
            color: "#d8d8d8"
            visible: control.checked
        }
    }

    contentItem: Text {
        anchors.fill: parent
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: parent.enabled ? "lightGray" : "darkGray"
        text: parent.text
    }
}
