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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editorwindow.h"

#include "editorapplication.h"
#include "editortab.h"
#include "objectlistmodel.h"
#include "openedproject.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EditorWindow::EditorWindow(EditorApplication& application)
  : QObject(&application),
    mApplication(application),
    mTitle(QString("LibrePCB %1").arg(qApp->applicationVersion())),
    mTabsModelLeft(new ObjectListModel(this)),
    mTabsModelRight(new ObjectListModel(this)),
    mEngine(new QQmlApplicationEngine(this)) {
  const QUrl url =
      Application::getResourcesDir().getPathTo("qml/MainWindow.qml").toQUrl();
  connect(
      mEngine.data(), &QQmlApplicationEngine::objectCreated, this,
      [url](QObject* obj, const QUrl& objUrl) {
        if ((!obj) && (objUrl == url)) {
          qFatal("Failed to load the QML window!");  // Quits the application!
        }
      },
      Qt::QueuedConnection);
  mEngine->rootContext()->setContextProperty("cppApp", &mApplication);
  mEngine->rootContext()->setContextProperty("cppWindow", this);
  mEngine->load(url);

  mTabsModelLeft->insert(0, std::make_shared<EditorTab>(mApplication, *this));
  mTabsModelLeft->insert(1, std::make_shared<EditorTab>(mApplication, *this));
  mTabsModelRight->insert(0, std::make_shared<EditorTab>(mApplication, *this));
}

EditorWindow::~EditorWindow() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QObject* EditorWindow::getCurrentProject() noexcept {
  return mCurrentProject.get();
}

QAbstractItemModel* EditorWindow::getTabsLeft() noexcept {
  return mTabsModelLeft.data();
}

QAbstractItemModel* EditorWindow::getTabsRight() noexcept {
  return mTabsModelRight.data();
}

/*******************************************************************************
 *  GUI Handlers
 ******************************************************************************/

bool EditorWindow::createProject() noexcept {
  if (auto p = mApplication.createProject()) {
    setCurrentProject(p);
    return true;
  }
  return false;
}

bool EditorWindow::openProject() noexcept {
  if (auto p = mApplication.openProject()) {
    setCurrentProject(p);
    return true;
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EditorWindow::setCurrentProject(
    std::shared_ptr<OpenedProject> p) noexcept {
  if (p != mCurrentProject) {
    mCurrentProject = p;
    emit currentProjectChanged();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
