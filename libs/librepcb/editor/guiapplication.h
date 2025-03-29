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

#ifndef LIBREPCB_EDITOR_GUIAPPLICATION_H
#define LIBREPCB_EDITOR_GUIAPPLICATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class LibraryEditor;
class LibraryManager;
class MainWindow;
class Notification;
class NotificationsModel;
class ProjectEditor;
class ProjectLibraryUpdater;
class QuickAccessModel;

/*******************************************************************************
 *  Class GuiApplication
 ******************************************************************************/

/**
 * @brief The GuiApplication class
 */
class GuiApplication final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  GuiApplication() = delete;
  GuiApplication(const GuiApplication& other) = delete;
  explicit GuiApplication(Workspace& ws, bool fileFormatIsOutdated,
                          QObject* parent = nullptr) noexcept;
  ~GuiApplication() noexcept;

  // Getters
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  NotificationsModel& getNotifications() noexcept { return *mNotifications; }
  QuickAccessModel& getQuickAccess() noexcept { return *mQuickAccessModel; }

  // General Methods
  void openFile(const FilePath& fp, QWidget* parent) noexcept;
  void switchWorkspace(QWidget* parent) noexcept;
  void execWorkspaceSettingsDialog(QWidget* parent) noexcept;
  void openLibraryManager() noexcept;
  void addExampleProjects(QWidget* parent) noexcept;
  void createProject(const FilePath& parentDir, bool eagleImport,
                     QWidget* parent) noexcept;
  /**
   * @brief Open a project with the editor (or bring an already opened editor to
   * front)
   *
   * @param filepath  The filepath to the *.lpp project file to open. If
   *                  invalid, a file dialog will be shown to select it.
   * @param parent    Parent widget for dialogs.
   *
   * @return The pointer to the opened project editor (nullptr on error)
   */
  ProjectEditor* openProject(FilePath filepath, QWidget* parent) noexcept;
  void createNewWindow(int id = -1) noexcept;
  bool requestClosingWindow(QWidget* parent) noexcept;
  void exec();
  void quit(QPointer<QWidget> parent) noexcept;

  // Operator Overloadings
  GuiApplication& operator=(const GuiApplication& rhs) = delete;

protected:
  bool eventFilter(QObject* watched, QEvent* event) noexcept override;

private:
  void openProjectsPassedByCommandLine() noexcept;
  void openProjectPassedByOs(const QString& file, bool silent = false) noexcept;

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

  /**
   * @brief Close all open project editors
   *
   * @param askForSave    If true, the user will be asked to save all modified
   *                      projects
   * @param parent        Parent for message boxes
   *
   * @retval  true if all projects successfully closed, false otherwise
   */
  bool closeAllProjects(bool askForSave, QWidget* parent) noexcept;

  void projectEditorClosed() noexcept;
  void openProjectLibraryUpdater(const FilePath& project) noexcept;

  void openLibraryEditor(const FilePath& libDir) noexcept;
  void libraryEditorDestroyed() noexcept;

  /**
   * @brief Close all open library editors
   *
   * @param askForSave    If true, the user will be asked to save all modified
   *                      libraries
   *
   * @retval  true if all library editors successfully closed, false otherwise
   */
  bool closeAllLibraryEditors(bool askForSave) noexcept;

  std::shared_ptr<MainWindow> getCurrentWindow() noexcept;
  void updateNoLibrariesInstalledNotification() noexcept;
  void updateDesktopIntegrationNotification() noexcept;

  Workspace& mWorkspace;
  std::shared_ptr<NotificationsModel> mNotifications;
  std::shared_ptr<Notification> mNotificationNoLibrariesInstalled;
  std::shared_ptr<Notification> mNotificationDesktopIntegration;
  std::shared_ptr<QuickAccessModel> mQuickAccessModel;
  QScopedPointer<LibraryManager> mLibraryManager;
  QHash<QString, ProjectEditor*> mOpenProjectEditors;
  QHash<FilePath, LibraryEditor*> mOpenLibraryEditors;
  std::unique_ptr<ProjectLibraryUpdater> mProjectLibraryUpdater;
  QList<std::shared_ptr<MainWindow>> mWindows;
  QTimer mSaveOpenedWindowsCountdown;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
