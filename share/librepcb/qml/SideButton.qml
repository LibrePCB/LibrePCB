import QtQuick 2.0
import QtQuick.Controls 2.15

AbstractButton {
    id: root
    implicitWidth: parent.width
    implicitHeight: parent.width
    checkable: true
    autoExclusive: true

    property alias iconSource: image.source
    property string name

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        property bool hasHover: false
        onEntered: hasHover = true
        onExited: hasHover = false
        onClicked: {
            if (root.checkable) {root.checked = true}
            root.onClicked()
        }
    }

    background: Rectangle {
        opacity: enabled ? 1 : 0.3
        color: root.checked ? "#1c1c1c" : (mouseArea.pressed ? "#555555" : (mouseArea.hasHover ? "#424242" : "#353535"))
        Rectangle {
            width: 3
            height: root.height
            color: "#d8d8d8"
            visible: root.checked
        }
    }

    contentItem: Item {
        anchors.fill: parent
        Image {
            id: image
            anchors.fill: parent
            anchors.margins: 10
        }
        ToolTip {
            x: parent.width + 5
            y: (parent.height - height) / 2
            delay: 1000
            timeout: 5000
            visible: hovered
            text: root.name
        }
    }
}
