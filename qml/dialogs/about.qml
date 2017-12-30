import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    width: layout.width + 12
    height: layout.height + 12

    ColumnLayout {
        id: layout
        spacing: 12

        Image {
            source: "qrc:/img/logo/64x64.png"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: qsTr("About LibrePCB")
            font.pointSize: 18
            font.bold: true
        }

        Text {
            text: qsTr("LibrePCB is a free & open source schematic/layout-editor.")
        }

        Text {
            text: qsTr("Version: %1 (%2)").arg(appVersion).arg(gitVersion)
        }

        Text {
            text: qsTr("Please see %1 for more information.").arg("<a href='http://librepcb.org/'>librepcb.org</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Text {
            text: qsTr("You can find the project on GitHub:<br>%1").arg("<a href='https://github.com/LibrePCB/LibrePCB'>https://github.com/LibrePCB/LibrePCB</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Button {
            text: "Close"
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: {
                view.close()
            }
        }
    }

}
