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

#ifndef LIBREPCB_PROJECT_CMDNETSIGNALEDIT_H
#define LIBREPCB_PROJECT_CMDNETSIGNALEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/circuitidentifier.h>
#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Circuit;
class NetSignal;

/*******************************************************************************
 *  Class CmdNetSignalSetName
 ******************************************************************************/

/**
 * @brief The CmdNetSignalSetName class
 */
class CmdNetSignalEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdNetSignalEdit(Circuit& circuit, NetSignal& netsignal) noexcept;
  ~CmdNetSignalEdit() noexcept;

  // Setters
  void setName(const CircuitIdentifier& name, bool isAutoName) noexcept;

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
  Circuit& mCircuit;
  NetSignal& mNetSignal;

  // General Attributes
  CircuitIdentifier mOldName;
  CircuitIdentifier mNewName;
  bool mOldIsAutoName;
  bool mNewIsAutoName;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
