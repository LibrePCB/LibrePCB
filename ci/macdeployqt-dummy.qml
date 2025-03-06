// Dummy QML because macdeployqt fails to bundle QtDBus if the application
// doesn't depend on QtQuick. Crap!!! And the following workaround doesn't work:
// https://stackoverflow.com/questions/73108273/building-and-distributing-a-macos-application-using-cmake/73126397

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
