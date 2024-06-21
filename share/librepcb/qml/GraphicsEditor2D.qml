import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import org.librepcb.qmlcomponents

Rectangle {
    id: root
    property alias scene: view.scene
    property color overlayColor: "#888888"

    color: "#4f4f4f"
    QuickGraphicsView {
        id: view
        objectName: "view"
        anchors.fill: parent
    }

    Column {
        x: parent.width - 8 - width
        y: 8
        width: 25

        ImageButton {
            width: parent.width
            icon: "qrc:///img/actions/icons8-zoom-in-30.png"
            color: root.overlayColor
            onClicked: view.zoomIn()
        }

        ImageButton {
            width: parent.width
            icon: "qrc:///img/actions/icons8-zoom-out-30.png"
            color: root.overlayColor
            onClicked: view.zoomOut()
        }

        ImageButton {
            width: parent.width
            icon: "qrc:///img/actions/icons8-zoom-to-extents-50.png"
            color: root.overlayColor
            onClicked: view.zoomAll()
        }
    }
}
