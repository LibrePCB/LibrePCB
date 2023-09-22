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

#ifndef LIBREPCB_GUI_EDITORWINDOW_H
#define LIBREPCB_GUI_EDITORWINDOW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtQuick>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace gui {

class EditorApplication;
class ObjectListModel;
class OpenedProject;

/*******************************************************************************
 *  Class EditorWindow
 ******************************************************************************/

/**
 * @brief Top level window GUI class
 */
class EditorWindow : public QObject {
  Q_OBJECT

public:
  Q_PROPERTY(QString title MEMBER mTitle NOTIFY titleChanged FINAL)
  Q_PROPERTY(QObject* currentProject READ getCurrentProject NOTIFY
                 currentProjectChanged FINAL)
  Q_PROPERTY(QAbstractItemModel* tabsLeft READ getTabsLeft CONSTANT FINAL)
  Q_PROPERTY(QAbstractItemModel* tabsRight READ getTabsRight CONSTANT FINAL)

  // Constructors / Destructor
  EditorWindow() = delete;
  EditorWindow(EditorApplication& application);
  EditorWindow(const EditorWindow& other) noexcept = delete;
  virtual ~EditorWindow() noexcept;

  // Getters
  QObject* getCurrentProject() noexcept;
  QAbstractItemModel* getTabsLeft() noexcept;
  QAbstractItemModel* getTabsRight() noexcept;

public slots:  // GUI Handlers
  bool createProject() noexcept;
  bool openProject() noexcept;

signals:
  void titleChanged(const QString& title);
  void currentProjectChanged();

private:  // Methods
  void setCurrentProject(std::shared_ptr<OpenedProject> p) noexcept;

private:
  EditorApplication& mApplication;
  QString mTitle;
  std::shared_ptr<OpenedProject> mCurrentProject;
  QScopedPointer<ObjectListModel> mTabsModelLeft;
  QScopedPointer<ObjectListModel> mTabsModelRight;
  QScopedPointer<QQmlApplicationEngine> mEngine;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb

#endif
