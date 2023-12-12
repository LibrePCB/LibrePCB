// Note: These dependencies require at least Qt 5.7 to run.
import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.2

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
