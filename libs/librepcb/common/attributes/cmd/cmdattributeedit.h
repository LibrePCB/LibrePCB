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

#ifndef LIBREPCB_CMDATTRIBUTEEDIT_H
#define LIBREPCB_CMDATTRIBUTEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"
#include "../attribute.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class CmdAttributeEdit
 ******************************************************************************/

/**
 * @brief The CmdAttributeEdit class
 */
class CmdAttributeEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdAttributeEdit() = delete;
  CmdAttributeEdit(const CmdAttributeEdit& other) = delete;
  explicit CmdAttributeEdit(Attribute& attribute) noexcept;
  ~CmdAttributeEdit() noexcept;

  // Setters
  void setKey(const AttributeKey& key) noexcept;
  void setType(const AttributeType& type) noexcept;
  void setValue(const QString& value) noexcept;
  void setUnit(const AttributeUnit* unit) noexcept;

  // Operator Overloadings
  CmdAttributeEdit& operator=(const CmdAttributeEdit& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  Attribute& mAttribute;

  // General Attributes
  AttributeKey mOldKey;
  AttributeKey mNewKey;
  const AttributeType* mOldType;
  const AttributeType* mNewType;
  QString mOldValue;
  QString mNewValue;
  const AttributeUnit* mOldUnit;
  const AttributeUnit* mNewUnit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_CMDATTRIBUTEEDIT_H
