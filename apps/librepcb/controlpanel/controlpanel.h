/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBREPCB_CONTROLPANEL_H
#define LIBREPCB_CONTROLPANEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class FilePath;

namespace library {
class Library;

namespace editor {
class LibraryEditor;
}

namespace manager {
class LibraryManager;
}
}

namespace project {
class Project;

namespace editor {
class ProjectEditor;
}
}

namespace workspace {
class Workspace;
}

namespace application {

namespace Ui {
class ControlPanel;
}

/*****************************************************************************************
 *  Class ControlPanel
 ****************************************************************************************/

/**
 * @brief The ControlPanel class
 *
 * @author ubruhin
 * @date 2014-06-23
 */
class ControlPanel final : public QMainWindow
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ControlPanel(workspace::Workspace& workspace);
        ~ControlPanel();


    public slots:

        void showControlPanel() noexcept;


    protected:

        // Inherited Methods
        virtual void closeEvent(QCloseEvent* event);


    private slots:

        // private slots
        void projectEditorClosed() noexcept;

        // Actions
        void on_actionAbout_triggered();
        void on_actionNew_Project_triggered();
        void on_actionOpen_Project_triggered();
        void on_actionOpen_Library_Manager_triggered();
        void on_actionClose_all_open_projects_triggered();
        void on_actionSwitch_Workspace_triggered();
        void on_projectTreeView_clicked(const QModelIndex& index);
        void on_projectTreeView_doubleClicked(const QModelIndex& index);
        void on_projectTreeView_customContextMenuRequested(const QPoint& pos);
        void on_recentProjectsListView_entered(const QModelIndex &index);
        void on_favoriteProjectsListView_entered(const QModelIndex &index);
        void on_recentProjectsListView_clicked(const QModelIndex &index);
        void on_favoriteProjectsListView_clicked(const QModelIndex &index);
        void on_recentProjectsListView_customContextMenuRequested(const QPoint &pos);
        void on_favoriteProjectsListView_customContextMenuRequested(const QPoint &pos);
        void on_actionRescanLibrary_triggered();


    private:

        // make some methods inaccessible...
        ControlPanel();
        ControlPanel(const ControlPanel& other);
        ControlPanel& operator=(const ControlPanel& rhs);


        // General private methods
        void saveSettings();
        void loadSettings();
        void showProjectReadmeInBrowser(const FilePath& projectFilePath) noexcept;

        // Project Management

        project::editor::ProjectEditor* newProject(const FilePath& parentDir) noexcept;

        /**
         * @brief Open a project with the editor (or bring an already opened editor to front)
         *
         * @param project       An already opened project (but without the editor)
         *
         * @return The pointer to the opened project editor (nullptr on error)
         */
        project::editor::ProjectEditor* openProject(project::Project& project) noexcept;

        /**
         * @brief Open a project with the editor (or bring an already opened editor to front)
         *
         * @param filepath  The filepath to the *.lpp project file to open
         *
         * @return The pointer to the opened project editor (nullptr on error)
         */
        project::editor::ProjectEditor* openProject(const FilePath& filepath) noexcept;

        /**
         * @brief Close an opened project editor
         *
         * @param editor        The reference to the open project editor
         * @param askForSave    If true, the user will be asked to save the projects
         *
         * @retval  true if the project was successfully closed, false otherwise
         */
        bool closeProject(project::editor::ProjectEditor& editor, bool askForSave) noexcept;

        /**
         * @brief Close an opened project editor
         *
         * @param filepath      The filepath to the *.lpp project file to close
         * @param askForSave    If true, the user will be asked to save the projects
         *
         * @retval  true if the project was successfully closed, false otherwise
         */
        bool closeProject(const FilePath& filepath, bool askForSave) noexcept;

        /**
         * @brief Close all open project editors
         *
         * @param askForSave    If true, the user will be asked to save all modified projects
         *
         * @retval  true if all projects successfully closed, false otherwise
         */
        bool closeAllProjects(bool askForSave) noexcept;

        /**
         * @brief Get the pointer to an already open project editor by its project filepath
         *
         * This method can also be used to check whether a project (by its filepath) is
         * already open or not.
         *
         * @param filepath  The filepath to a *.lpp project file
         *
         * @return The pointer to the open project editor, or nullptr if the project is not open
         */
        project::editor::ProjectEditor* getOpenProject(const FilePath& filepath) const noexcept;


        // Library Management
        void openLibraryEditor(QSharedPointer<library::Library> lib) noexcept;
        void libraryEditorDestroyed() noexcept;

        /**
         * @brief Close all open library editors
         *
         * @param askForSave    If true, the user will be asked to save all modified libraries
         *
         * @retval  true if all library editors successfully closed, false otherwise
         */
        bool closeAllLibraryEditors(bool askForSave) noexcept;

        // Attributes
        workspace::Workspace& mWorkspace;
        QScopedPointer<Ui::ControlPanel> mUi;
        QScopedPointer<library::manager::LibraryManager> mLibraryManager;
        QHash<QString, project::editor::ProjectEditor*> mOpenProjectEditors;
        QHash<library::Library*, library::editor::LibraryEditor*> mOpenLibraryEditors;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace application
} // namespace librepcb

#endif // LIBREPCB_CONTROLPANEL_H
