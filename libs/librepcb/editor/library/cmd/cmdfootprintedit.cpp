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
#include "cmdfootprintedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdFootprintEdit::CmdFootprintEdit(Footprint& fpt) noexcept
  : UndoCommand(tr("Edit footprint")),
    mFootprint(fpt),
    mOldName(fpt.getNames().getDefaultValue()),
    mNewName(mOldName),
    mOldModelPosition(fpt.getModelPosition()),
    mNewModelPosition(mOldModelPosition),
    mOldModelRotation(fpt.getModelRotation()),
    mNewModelRotation(mOldModelRotation),
    mOldModels(fpt.getModels()),
    mNewModels(mOldModels) {
}

CmdFootprintEdit::~CmdFootprintEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdFootprintEdit::setName(const ElementName& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

void CmdFootprintEdit::setModelPosition(const Point3D& pos) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewModelPosition = pos;
}

void CmdFootprintEdit::setModelRotation(const Angle3D& rot) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewModelRotation = rot;
}

void CmdFootprintEdit::setModels(const QSet<Uuid>& models) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewModels = models;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFootprintEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewModelPosition != mOldModelPosition) return true;
  if (mNewModelRotation != mOldModelRotation) return true;
  if (mNewModels != mOldModels) return true;
  return false;
}

void CmdFootprintEdit::performUndo() {
  mFootprint.getNames().setDefaultValue(mOldName);
  mFootprint.setModelPosition(mOldModelPosition);
  mFootprint.setModelRotation(mOldModelRotation);
  mFootprint.setModels(mOldModels);
}

void CmdFootprintEdit::performRedo() {
  mFootprint.getNames().setDefaultValue(mNewName);
  mFootprint.setModelPosition(mNewModelPosition);
  mFootprint.setModelRotation(mNewModelRotation);
  mFootprint.setModels(mNewModels);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
