import QtQuick 2.0
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.2

ColumnLayout {
    required property QtObject cppTabsModel

    spacing: 0

    Row {
        height: 20
        spacing: 5
        Repeater {
            model: cppTabsModel
            Rectangle {
                color: "#ff0000"
                height: childrenRect.height
                width: childrenRect.width
                Label {
                    text: model.item.title
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#4f4f4f"
    }
}
