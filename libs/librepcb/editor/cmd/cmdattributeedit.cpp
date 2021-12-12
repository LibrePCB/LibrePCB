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
#include "cmdattributeedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdAttributeEdit::CmdAttributeEdit(Attribute& attribute) noexcept
  : UndoCommand(tr("Edit circle")),
    mAttribute(attribute),
    mOldKey(attribute.getKey()),
    mNewKey(mOldKey),
    mOldType(&attribute.getType()),
    mNewType(mOldType),
    mOldValue(attribute.getValue()),
    mNewValue(mOldValue),
    mOldUnit(attribute.getUnit()),
    mNewUnit(mOldUnit) {
}

CmdAttributeEdit::~CmdAttributeEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdAttributeEdit::setKey(const AttributeKey& key) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewKey = key;
}

void CmdAttributeEdit::setType(const AttributeType& type) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewType = &type;
}

void CmdAttributeEdit::setValue(const QString& value) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewValue = value;
}

void CmdAttributeEdit::setUnit(const AttributeUnit* unit) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewUnit = unit;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdAttributeEdit::performExecute() {
  bool modified = false;
  // Set type/value/unit before key for exception safety (setTypeValueUnit() can
  // throw, but setKey() not)!
  if (mAttribute.setTypeValueUnit(*mNewType, mNewValue, mNewUnit)) {
    modified = true;
  }
  if (mAttribute.setKey(mNewKey)) {
    modified = true;
  }
  return modified;
}

void CmdAttributeEdit::performUndo() {
  mAttribute.setTypeValueUnit(*mOldType, mOldValue, mOldUnit);
  mAttribute.setKey(mOldKey);
}

void CmdAttributeEdit::performRedo() {
  mAttribute.setTypeValueUnit(*mNewType, mNewValue, mNewUnit);
  mAttribute.setKey(mNewKey);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
