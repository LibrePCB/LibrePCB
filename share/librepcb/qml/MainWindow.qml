import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2

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
            Action { text: qsTr("&New...") }
            Action { text: qsTr("&Open...") }
            Action { text: qsTr("&Save") }
            Action { text: qsTr("Save &As...") }
            Action { text: qsTr("&Quit") }
        }
        Menu {
            title: qsTr("&Extras")
            Action {
                text: qsTr("Workspace Settings") + "..."
                icon.source: "qrc:///img/actions/settings.png"
                shortcut: "Ctrl+,"
                onTriggered: cppApp.openWorkspaceSettings()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action { text: qsTr("&About") }
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
                    iconSource: "qrc:///img/actions/libraries.png"
                    checkable: false
                    autoExclusive: false
                    onClicked: cppApp.openLibraryManager()
                }
                SideButton {
                    id: btnProject
                    name: qsTr("Projects")
                    iconSource: "qrc:///img/actions/projects.png"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnChecks
                    name: qsTr("Checks")
                    iconSource: "qrc:///img/actions/checks.png"
                    enabled: !cppApp.openedProjects.empty
                }
                SideButton {
                    id: btnSearch
                    name: qsTr("Find")
                    iconSource: "qrc:///img/actions/find.png"
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
                SplitView.minimumWidth: 100
                SplitView.preferredWidth: 300
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
            SchematicEditor {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumWidth: 100
            }
        }
    }

    footer: ToolBar {
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
