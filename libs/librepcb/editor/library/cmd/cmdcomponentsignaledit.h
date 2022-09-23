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

#ifndef LIBREPCB_EDITOR_CMDCOMPONENTSIGNALEDIT_H
#define LIBREPCB_EDITOR_CMDCOMPONENTSIGNALEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/cmp/componentsignal.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdComponentSignalEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentSignalEdit class
 */
class CmdComponentSignalEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdComponentSignalEdit() = delete;
  CmdComponentSignalEdit(const CmdComponentSignalEdit& other) = delete;
  explicit CmdComponentSignalEdit(ComponentSignal& signal) noexcept;
  ~CmdComponentSignalEdit() noexcept;

  // Setters
  void setName(const CircuitIdentifier& name) noexcept;
  void setRole(const SignalRole& role) noexcept;
  void setForcedNetName(const QString& name) noexcept;
  void setIsRequired(bool required) noexcept;
  void setIsNegated(bool negated) noexcept;
  void setIsClock(bool clock) noexcept;

  // Operator Overloadings
  CmdComponentSignalEdit& operator=(const CmdComponentSignalEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  ComponentSignal& mSignal;

  CircuitIdentifier mOldName;
  CircuitIdentifier mNewName;
  SignalRole mOldRole;
  SignalRole mNewRole;
  QString mOldForcedNetName;
  QString mNewForcedNetName;
  bool mOldIsRequired;
  bool mNewIsRequired;
  bool mOldIsNegated;
  bool mNewIsNegated;
  bool mOldIsClock;
  bool mNewIsClock;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdComponentSignalInsert =
    CmdListElementInsert<ComponentSignal, ComponentSignalListNameProvider,
                         ComponentSignal::Event>;
using CmdComponentSignalRemove =
    CmdListElementRemove<ComponentSignal, ComponentSignalListNameProvider,
                         ComponentSignal::Event>;
using CmdComponentSignalsSwap =
    CmdListElementsSwap<ComponentSignal, ComponentSignalListNameProvider,
                        ComponentSignal::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
