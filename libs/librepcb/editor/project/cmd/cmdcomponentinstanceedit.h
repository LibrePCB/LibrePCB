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

#ifndef LIBREPCB_EDITOR_CMDCOMPONENTINSTANCEEDIT_H
#define LIBREPCB_EDITOR_CMDCOMPONENTINSTANCEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/types/circuitidentifier.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class Component;
class ComponentInstance;
class ComponentSymbolVariant;

namespace editor {

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
  void setAssemblyOptions(const ComponentAssemblyOptionList& options) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  Circuit& mCircuit;
  ComponentInstance& mComponentInstance;

  // Misc
  CircuitIdentifier mOldName;
  CircuitIdentifier mNewName;
  QString mOldValue;
  QString mNewValue;
  AttributeList mOldAttributes;
  AttributeList mNewAttributes;
  ComponentAssemblyOptionList mOldAssemblyOptions;
  ComponentAssemblyOptionList mNewAssemblyOptions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
