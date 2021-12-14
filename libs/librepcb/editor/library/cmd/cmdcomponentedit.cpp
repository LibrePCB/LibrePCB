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
#include "cmdcomponentedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdComponentEdit::CmdComponentEdit(Component& component) noexcept
  : CmdLibraryElementEdit(component, tr("Edit component metadata")),
    mComponent(component),
    mOldSchematicOnly(component.isSchematicOnly()),
    mNewSchematicOnly(mOldSchematicOnly),
    mOldDefaultValue(component.getDefaultValue()),
    mNewDefaultValue(mOldDefaultValue),
    mOldPrefixes(component.getPrefixes()),
    mNewPrefixes(mOldPrefixes) {
}

CmdComponentEdit::~CmdComponentEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdComponentEdit::setIsSchematicOnly(bool schematicOnly) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSchematicOnly = schematicOnly;
}

void CmdComponentEdit::setDefaultValue(const QString& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDefaultValue = value;
}

void CmdComponentEdit::setPrefix(const QString& norm,
                                 const ComponentPrefix& prefix) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPrefixes.insert(norm, prefix);
}

void CmdComponentEdit::setPrefixes(
    const NormDependentPrefixMap& prefixes) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPrefixes = prefixes;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdComponentEdit::performExecute() {
  if (CmdLibraryElementEdit::performExecute()) return true;  // can throw
  if (mNewSchematicOnly != mOldSchematicOnly) return true;
  if (mNewDefaultValue != mOldDefaultValue) return true;
  if (mNewPrefixes != mOldPrefixes) return true;
  return false;
}

void CmdComponentEdit::performUndo() {
  CmdLibraryElementEdit::performUndo();  // can throw
  mComponent.setIsSchematicOnly(mOldSchematicOnly);
  mComponent.setDefaultValue(mOldDefaultValue);
  mComponent.setPrefixes(mOldPrefixes);
}

void CmdComponentEdit::performRedo() {
  CmdLibraryElementEdit::performRedo();  // can throw
  mComponent.setIsSchematicOnly(mNewSchematicOnly);
  mComponent.setDefaultValue(mNewDefaultValue);
  mComponent.setPrefixes(mNewPrefixes);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
