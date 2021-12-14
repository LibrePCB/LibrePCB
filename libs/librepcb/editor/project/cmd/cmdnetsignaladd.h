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

#ifndef LIBREPCB_EDITOR_CMDNETSIGNALADD_H
#define LIBREPCB_EDITOR_CMDNETSIGNALADD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/types/circuitidentifier.h>

#include <QtCore>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class NetClass;
class NetSignal;

namespace editor {

/*******************************************************************************
 *  Class CmdNetSignalAdd
 ******************************************************************************/

/**
 * @brief The CmdNetSignalAdd class
 */
class CmdNetSignalAdd final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdNetSignalAdd(
      Circuit& circuit, NetClass& netclass,
      const tl::optional<CircuitIdentifier>& name = tl::nullopt) noexcept;
  ~CmdNetSignalAdd() noexcept;

  // Getters
  NetSignal* getNetSignal() const noexcept { return mNetSignal; }

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  Circuit& mCircuit;
  NetClass& mNetClass;
  tl::optional<CircuitIdentifier> mName;
  NetSignal* mNetSignal;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
