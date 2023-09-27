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

#ifndef LIBREPCB_GUI_OPENEDPROJECT_H
#define LIBREPCB_GUI_OPENEDPROJECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

namespace gui {

class EditorApplication;
class ObjectListModel;

/*******************************************************************************
 *  Class OpenedProject
 ******************************************************************************/

/**
 * @brief Wrapper of an opened project
 */
class OpenedProject : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  OpenedProject() = delete;
  OpenedProject(EditorApplication& application,
                std::unique_ptr<Project> project) noexcept;
  OpenedProject(const OpenedProject& other) noexcept = delete;
  virtual ~OpenedProject() noexcept;

  // Properties
  Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
  Q_PROPERTY(QAbstractItemModel* schematics READ getSchematics CONSTANT)
  Q_PROPERTY(QAbstractItemModel* boards READ getBoards CONSTANT)

  // Getters
  const QString& getName() const noexcept;
  QAbstractItemModel* getSchematics() noexcept;
  QAbstractItemModel* getBoards() noexcept;

signals:
  void nameChanged(const QString& name);

private:
  EditorApplication& mApplication;
  std::unique_ptr<Project> mProject;
  QScopedPointer<ObjectListModel> mSchematicsModel;
  QScopedPointer<ObjectListModel> mBoardsModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb

#endif
