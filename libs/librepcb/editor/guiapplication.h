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

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {
namespace app {

class LibrariesModel;
class MainWindow;
class Notification;
class NotificationsModel;
class ProjectsModel;
class QuickAccessModel;

/*******************************************************************************
 *  Class GuiApplication
 ******************************************************************************/

/**
 * @brief The GuiApplication class
 */
class GuiApplication : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  GuiApplication() = delete;
  GuiApplication(const GuiApplication& other) = delete;
  explicit GuiApplication(Workspace& ws, bool fileFormatIsOutdated,
                          QObject* parent = nullptr) noexcept;
  virtual ~GuiApplication() noexcept;

  // Getters
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  NotificationsModel& getNotifications() noexcept { return *mNotifications; }
  ProjectsModel& getProjects() noexcept { return *mProjects; }
  QuickAccessModel& getQuickAccess() noexcept { return *mQuickAccessModel; }

  // General Methods
  bool actionTriggered(ui::ActionId id, int sectionIndex) noexcept;
  void createNewWindow(int id = -1, int projectIndex = -1) noexcept;
  bool requestClosingWindow() noexcept;
  void exec();

  // Operator Overloadings
  GuiApplication& operator=(const GuiApplication& rhs) = delete;

private:
  QString buildAppVersionDetails() const noexcept;
  std::shared_ptr<MainWindow> getCurrentWindow() noexcept;
  void updateNoLibrariesInstalledNotification() noexcept;
  void updateDesktopIntegrationNotification() noexcept;
  bool requestClosingAllProjects() noexcept;

  Workspace& mWorkspace;
  std::shared_ptr<NotificationsModel> mNotifications;
  std::shared_ptr<Notification> mNotificationNoLibrariesInstalled;
  std::shared_ptr<Notification> mNotificationDesktopIntegration;
  std::shared_ptr<QuickAccessModel> mQuickAccessModel;
  std::shared_ptr<LibrariesModel> mLibraries;
  std::shared_ptr<slint::FilterModel<ui::LibraryData>> mLibrariesFiltered;
  std::shared_ptr<ProjectsModel> mProjects;
  QList<std::shared_ptr<MainWindow>> mWindows;
  QTimer mSaveOpenedWindowsCountdown;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
