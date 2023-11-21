import QtQuick 2.0
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.2
import org.librepcb.qmlcomponents 1.0

Rectangle {
    color: "#4f4f4f"
    OpenGlView {
        id: view
        anchors.fill: parent
    }

    Column {
        x: parent.width - 8 - width
        y: 8
        width: 25

        ImageButton {
            width: parent.width
            icon: "qrc:///img/actions/icons8-zoom-in-30.png"
            onClicked: view.zoomIn()
        }

        ImageButton {
            width: parent.width
            icon: "qrc:///img/actions/icons8-zoom-out-30.png"
            onClicked: view.zoomOut()
        }

        ImageButton {
            width: parent.width
            icon: "qrc:///img/actions/icons8-zoom-to-extents-50.png"
            onClicked: view.zoomAll()
        }
    }
}
