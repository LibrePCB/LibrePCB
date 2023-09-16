import QtQuick 2.0
import QtQuick.Controls 1.5
import QtQuick.Controls 2.0 as Controls2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.0

ApplicationWindow {
    id: window
    width: 1024
    height: 768
    visible: true
    title: cppWindow.title
    color: "#4f4f4f"

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            MenuItem { text: qsTr("&New...") }
            MenuItem { text: qsTr("&Open...") }
            MenuItem { text: qsTr("&Save") }
            MenuItem { text: qsTr("Save &As...") }
            MenuItem { text: qsTr("&Quit") }
        }
        Menu {
            title: qsTr("&Edit")
            MenuItem { text: qsTr("Cu&t") }
            MenuItem { text: qsTr("&Copy") }
            MenuItem { text: qsTr("&Paste") }
        }
        Menu {
            title: qsTr("&Help")
            MenuItem { text: qsTr("&About") }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            Layout.preferredWidth: 50
            Layout.fillHeight: true
            color: "#353535"
            visible: true
            Column {
                anchors.fill: parent
                SideButton {
                    id: btnHome
                    text: "HOME"
                    checked: true
                }
                SideButton {
                    id: btnLibraries
                    text: "LIBS"
                }
                SideButton {
                    id: btnProject
                    text: "PROJECT"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnLayers
                    text: "LAYERS"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnChecks
                    text: "CHECKS"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnSearch
                    text: "FIND"
                    enabled: !cppApp.openedProjects.empty
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: btnHome.checked
            Button {
                text: "Create Project"
                Layout.alignment: Qt.AlignCenter
                onClicked: cppApp.createProject()
            }
            Button {
                text: "Open Project"
                Layout.alignment: Qt.AlignCenter
                onClicked: cppApp.openProject()
            }
        }
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !btnHome.checked
            Column {
                Layout.minimumWidth: 100
                width: 300
                ProjectPanel {
                    width: parent.width
                    height: parent.height
                    visible: btnProject.checked
                }
                LibrariesPanel {
                    width: parent.width
                    height: parent.height
                    visible:btnLibraries.checked
                }
            }
            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                EditorArea {
                    Layout.minimumWidth: 100
                    width: parent.width / 2
                    cppTabsModel: cppWindow.tabsLeft
                }
                EditorArea {
                    Layout.minimumWidth: 100
                    width: parent.width / 2
                    cppTabsModel: cppWindow.tabsRight
                }
            }
        }
    }

    statusBar: StatusBar {
        implicitHeight: 22
        RowLayout {
            anchors.fill: parent
            Label {
                text: "Workspace: " + cppApp.wsPath
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                Layout.leftMargin: 6
                Layout.fillWidth: true
            }
        }
    }
}
