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
#include "openedproject.h"

#include "boardgui.h"
#include "editorapplication.h"
#include "objectlistmodel.h"
#include "schematicgui.h"

#include <librepcb/core/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenedProject::OpenedProject(EditorApplication& application,
                             std::unique_ptr<Project> project) noexcept
  : QObject(&application),
    mApplication(application),
    mProject(std::move(project)),
    mSchematicsModel(new ObjectListModel(this)),
    mBoardsModel(new ObjectListModel(this)) {
  Q_ASSERT(mProject);
  foreach (Schematic* s, mProject->getSchematics()) {
    mSchematicsModel->insert(-1, std::make_shared<SchematicGui>(*this, *s));
  }
  foreach (Board* b, mProject->getBoards()) {
    mBoardsModel->insert(-1, std::make_shared<BoardGui>(*this, *b));
  }
}

OpenedProject::~OpenedProject() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QString& OpenedProject::getName() const noexcept {
  return *mProject->getName();
}

QAbstractItemModel* OpenedProject::getSchematics() noexcept {
  return mSchematicsModel.data();
}

QAbstractItemModel* OpenedProject::getBoards() noexcept {
  return mBoardsModel.data();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
