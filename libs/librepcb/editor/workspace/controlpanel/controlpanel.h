/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

#ifndef LIBREPCB_EDITOR_CONTROLPANEL_H
#define LIBREPCB_EDITOR_CONTROLPANEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class Library;
class Project;
class Workspace;

namespace editor {

class FavoriteProjectsModel;
class LibraryEditor;
class LibraryManager;
class ProjectEditor;
class ProjectLibraryUpdater;
class ProjectTreeModel;
class RecentProjectsModel;
class StandardEditorCommandHandler;

namespace Ui {
class ControlPanel;
}

/*******************************************************************************
 *  Class ControlPanel
 ******************************************************************************/

/**
 * @brief The ControlPanel class
 */
class ControlPanel final : public QMainWindow {
  Q_OBJECT

  // ProjectLibraryUpdater needs access to openProject() and getOpenProject()
  friend class ProjectLibraryUpdater;

public:
  // Constructors / Destructor
  ControlPanel() = delete;
  ControlPanel(const ControlPanel& other) = delete;
  explicit ControlPanel(Workspace& workspace, bool fileFormatIsOutdated);
  virtual ~ControlPanel() noexcept;

  // Operator Overloadings
  ControlPanel& operator=(const ControlPanel& rhs) = delete;

public slots:
  void showControlPanel() noexcept;
  void openProjectLibraryUpdater(const FilePath& project) noexcept;

protected:
  // Inherited Methods
  virtual void closeEvent(QCloseEvent* event) override;
  virtual bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
  // private slots
  void openProjectsPassedByCommandLine() noexcept;
  void openProjectPassedByOs(const QString& file, bool silent = false) noexcept;
  void projectEditorClosed() noexcept;

  // Actions
  void on_projectTreeView_clicked(const QModelIndex& index);
  void on_projectTreeView_doubleClicked(const QModelIndex& index);
  void on_projectTreeView_customContextMenuRequested(const QPoint& pos);
  void on_recentProjectsListView_entered(const QModelIndex& index);
  void on_favoriteProjectsListView_entered(const QModelIndex& index);
  void on_recentProjectsListView_clicked(const QModelIndex& index);
  void on_favoriteProjectsListView_clicked(const QModelIndex& index);
  void on_recentProjectsListView_customContextMenuRequested(const QPoint& pos);
  void on_favoriteProjectsListView_customContextMenuRequested(
      const QPoint& pos);

private:
  // General private methods
  void createActions() noexcept;
  void createMenus() noexcept;
  void saveSettings();
  void loadSettings();
  void updateDesktopIntegrationMessage() noexcept;
  void openLibraryManager() noexcept;
  void switchWorkspace() noexcept;
  void showProjectReadmeInBrowser(const FilePath& projectFilePath) noexcept;

  // Project Management

  ProjectEditor* newProject(bool eagleImport = false,
                            FilePath parentDir = FilePath()) noexcept;

  /**
   * @brief Open a project with the editor (or bring an already opened editor to
   * front)
   *
   * @param filepath  The filepath to the *.lpp project file to open. If invalid
   *                  (the default), a file dialog will be shown to select it.
   *
   * @return The pointer to the opened project editor (nullptr on error)
   */
  ProjectEditor* openProject(FilePath filepath = FilePath()) noexcept;

  /**
   * @brief Close an opened project editor
   *
   * @param editor        The reference to the open project editor
   * @param askForSave    If true, the user will be asked to save the projects
   *
   * @retval  true if the project was successfully closed, false otherwise
   */
  bool closeProject(ProjectEditor& editor, bool askForSave) noexcept;

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
   * @param askForSave    If true, the user will be asked to save all modified
   * projects
   *
   * @retval  true if all projects successfully closed, false otherwise
   */
  bool closeAllProjects(bool askForSave) noexcept;

  /**
   * @brief Get the pointer to an already open project editor by its project
   * filepath
   *
   * This method can also be used to check whether a project (by its filepath)
   * is already open or not.
   *
   * @param filepath  The filepath to a *.lpp project file
   *
   * @return The pointer to the open project editor, or nullptr if the project
   * is not open
   */
  ProjectEditor* getOpenProject(const FilePath& filepath) const noexcept;

  /**
   * @brief Ask the user whether to restore a backup of a project
   *
   * @param dir   The project directory to be restored.
   *
   * @retval true   Restore backup.
   * @retval false  Do not restore backup.
   *
   * @throw Exception to abort opening the project.
   */
  static bool askForRestoringBackup(const FilePath& dir);

  // Library Management
  void openLibraryEditor(const FilePath& libDir) noexcept;
  void libraryEditorDestroyed() noexcept;

  /**
   * @brief Close all open library editors
   *
   * @param askForSave    If true, the user will be asked to save all modified
   * libraries
   *
   * @retval  true if all library editors successfully closed, false otherwise
   */
  bool closeAllLibraryEditors(bool askForSave) noexcept;

  // Attributes
  Workspace& mWorkspace;
  QScopedPointer<Ui::ControlPanel> mUi;
  QScopedPointer<StandardEditorCommandHandler> mStandardCommandHandler;
  QScopedPointer<ProjectTreeModel> mProjectTreeModel;
  QScopedPointer<RecentProjectsModel> mRecentProjectsModel;
  QScopedPointer<FavoriteProjectsModel> mFavoriteProjectsModel;
  QScopedPointer<LibraryManager> mLibraryManager;
  QHash<QString, ProjectEditor*> mOpenProjectEditors;
  QHash<FilePath, LibraryEditor*> mOpenLibraryEditors;
  QScopedPointer<ProjectLibraryUpdater> mProjectLibraryUpdater;

  // Actions
  QScopedPointer<QAction> mActionLibraryManager;
  QScopedPointer<QAction> mActionWorkspaceSettings;
  QScopedPointer<QAction> mActionRescanLibraries;
  QScopedPointer<QAction> mActionSwitchWorkspace;
  QScopedPointer<QAction> mActionNewProject;
  QScopedPointer<QAction> mActionOpenProject;
  QScopedPointer<QAction> mActionCloseAllProjects;
  QScopedPointer<QAction> mActionImportEagleProject;
  QScopedPointer<QAction> mActionAboutLibrePcb;
  QScopedPointer<QAction> mActionAboutQt;
  QScopedPointer<QAction> mActionOnlineDocumentation;
  QScopedPointer<QAction> mActionKeyboardShortcutsReference;
  QScopedPointer<QAction> mActionWebsite;
  QScopedPointer<QAction> mActionQtQuickTest;
  QScopedPointer<QAction> mActionQuit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
