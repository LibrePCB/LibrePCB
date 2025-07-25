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
#include "appwindow.h"
#include "utils/uiobjectlist.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class GraphicsLayerList;
class LibrariesModel;
class LibraryEditor;
class LibraryElementCache;
class MainWindow;
class Notification;
class NotificationsModel;
class ProjectEditor;
class ProjectLibraryUpdater;
class QuickAccessModel;
class SlintKeyEventTextBuilder;

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

  // Workspace
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  QuickAccessModel& getQuickAccess() noexcept { return *mQuickAccessModel; }
  void openFile(const FilePath& fp, QWidget* parent) noexcept;
  void switchWorkspace(QWidget* parent) noexcept;
  void execWorkspaceSettingsDialog(QWidget* parent) noexcept;
  void addExampleProjects(QWidget* parent) noexcept;

  // General
  const GraphicsLayerList& getPreviewLayers() const noexcept {
    return *mPreviewLayers;
  }
  const LibraryElementCache& getLibraryElementCache() const noexcept {
    return *mLibraryElementCache;
  }

  // Libraries
  LibrariesModel& getLocalLibraries() noexcept { return *mLocalLibraries; }
  LibrariesModel& getRemoteLibraries() noexcept { return *mRemoteLibraries; }
  const QVector<std::shared_ptr<LibraryEditor>>& getLibraries() noexcept {
    return mLibraries->values();
  }
  std::shared_ptr<LibraryEditor> getLibrary(const FilePath& libDir) noexcept;
  std::shared_ptr<LibraryEditor> openLibrary(const FilePath& libDir) noexcept;
  void closeLibrary(const FilePath& libDir) noexcept;
  bool requestClosingAllLibraries() noexcept;

  // Projects
  const QVector<std::shared_ptr<ProjectEditor>>& getProjects() noexcept {
    return mProjects->values();
  }
  void createProject(const FilePath& parentDir, bool eagleImport,
                     QWidget* parent) noexcept;
  /**
   * @brief Open a project with the editor
   *
   * @param fp        The filepath to the *.lpp project file to open. If
   *                  invalid, a file dialog will be shown to select it.
   * @param parent    Parent widget for dialogs.
   *
   * @return The pointer to the opened project editor (nullptr on error)
   */
  std::shared_ptr<ProjectEditor> openProject(FilePath fp,
                                             QWidget* parent) noexcept;
  void closeProject(int index) noexcept;
  bool requestClosingAllProjects() noexcept;

  // Window Management
  NotificationsModel& getNotifications() noexcept { return *mNotifications; }
  void createNewWindow(int id = -1, int projectIndex = -1) noexcept;
  int getWindowCount() const noexcept;
  void stopWindowStateAutosaveTimer() noexcept;

  // General Methods
  void exec();
  void quit(QPointer<QWidget> parent) noexcept;

  // Operator Overloadings
  GuiApplication& operator=(const GuiApplication& rhs) = delete;

signals:
  void statusBarMessageChanged(const QString& message, int timeoutMs);
  void librariesContainStandardComponentsChanged(bool contains);

protected:
  bool eventFilter(QObject* watched, QEvent* event) noexcept override;

private:
  void openProjectsPassedByCommandLine() noexcept;
  void openProjectPassedByOs(const QString& file, bool silent = false) noexcept;
  void openProjectLibraryUpdater(const FilePath& project) noexcept;

  std::shared_ptr<MainWindow> getCurrentWindow() noexcept;
  void updateLibrariesContainStandardComponents() noexcept;
  void updateNoLibrariesInstalledNotification() noexcept;
  void updateDesktopIntegrationNotification() noexcept;

  Workspace& mWorkspace;
  bool mLibrariesContainStandardComponents;
  std::unique_ptr<GraphicsLayerList> mPreviewLayers;
  std::shared_ptr<LibraryElementCache> mLibraryElementCache;
  std::shared_ptr<NotificationsModel> mNotifications;
  std::shared_ptr<Notification> mNotificationNoLibrariesInstalled;
  std::shared_ptr<Notification> mNotificationDesktopIntegration;
  std::shared_ptr<QuickAccessModel> mQuickAccessModel;
  std::shared_ptr<LibrariesModel> mLocalLibraries;
  std::shared_ptr<LibrariesModel> mRemoteLibraries;
  std::unique_ptr<SlintKeyEventTextBuilder> mLibrariesFilter;
  std::shared_ptr<UiObjectList<ProjectEditor, ui::ProjectData>> mProjects;
  std::shared_ptr<UiObjectList<LibraryEditor, ui::LibraryData>> mLibraries;
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
