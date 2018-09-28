/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEEDIT_H
#define LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/attributes/attribute.h>
#include <librepcb/common/circuitidentifier.h>
#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace library {
class Component;
class ComponentSymbolVariant;
}  // namespace library

namespace project {

class Circuit;
class ComponentInstance;

/*******************************************************************************
 *  Class CmdComponentInstanceEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentInstanceEdit class
 */
class CmdComponentInstanceEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdComponentInstanceEdit(Circuit& circuit, ComponentInstance& cmp) noexcept;
  ~CmdComponentInstanceEdit() noexcept;

  // Setters
  void setName(const CircuitIdentifier& name) noexcept;
  void setValue(const QString& value) noexcept;
  void setAttributes(const AttributeList& attributes) noexcept;

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
  Circuit&           mCircuit;
  ComponentInstance& mComponentInstance;

  // Misc
  CircuitIdentifier mOldName;
  CircuitIdentifier mNewName;
  QString           mOldValue;
  QString           mNewValue;
  AttributeList     mOldAttributes;
  AttributeList     mNewAttributes;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEEDIT_H
