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
            title: qsTr("&Extras")
            MenuItem {
                text: qsTr("Workspace Settings") + "..."
                iconSource: "qrc:///img/actions/settings.png"
                shortcut: "Ctrl+,"
                onTriggered: cppApp.openWorkspaceSettings()
            }
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
                    name: qsTr("Home")
                    iconSource: "qrc:///img/actions/home.svg"
                    checked: true
                }
                SideButton {
                    id: btnLibraries
                    name: qsTr("Libraries")
                    iconSource: "qrc:///img/actions/libraries.svg"
                }
                SideButton {
                    id: btnProject
                    name: qsTr("Projects")
                    iconSource: "qrc:///img/actions/projects.svg"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnChecks
                    name: qsTr("Checks")
                    iconSource: "qrc:///img/actions/checks.svg"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnSearch
                    name: qsTr("Find")
                    iconSource: "qrc:///img/actions/find.svg"
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
                onClicked: if (cppWindow.createProject()) {btnProject.checked = true}
            }
            Button {
                text: "Open Project"
                Layout.alignment: Qt.AlignCenter
                onClicked: if (cppWindow.openProject()) {btnProject.checked = true}
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
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 100
                color: "#4f4f4f"
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
