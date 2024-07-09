import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2

ColumnLayout {
    anchors.fill: parent

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        Text {
            anchors.centerIn: parent
            text: "Hello from QML!"
            font.pointSize: 24
            color: "green"
        }

    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        Text {
            anchors.centerIn: parent
            text: "If you can read this text, it works!"
            color: "green"
        }

    }

    Button {
        Layout.fillWidth: true
        Layout.margins: 8
        text: "Close"
        onClicked: view.close()
    }

}
