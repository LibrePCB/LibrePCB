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

#ifndef LIBREPCB_EDITOR_CMDCOMPONENTPINSIGNALMAPITEMEDIT_H
#define LIBREPCB_EDITOR_CMDCOMPONENTPINSIGNALMAPITEMEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/library/cmp/cmpsigpindisplaytype.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentPinSignalMapItem;

namespace editor {

/*******************************************************************************
 *  Class CmdComponentPinSignalMapItemEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentPinSignalMapItemEdit class
 */
class CmdComponentPinSignalMapItemEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdComponentPinSignalMapItemEdit() = delete;
  CmdComponentPinSignalMapItemEdit(
      const CmdComponentPinSignalMapItemEdit& other) = delete;
  explicit CmdComponentPinSignalMapItemEdit(
      ComponentPinSignalMapItem& item) noexcept;
  ~CmdComponentPinSignalMapItemEdit() noexcept;

  // Setters
  void setSignalUuid(const std::optional<Uuid>& uuid) noexcept;
  void setDisplayType(const CmpSigPinDisplayType& type) noexcept;

  // Operator Overloadings
  CmdComponentPinSignalMapItemEdit& operator=(
      const CmdComponentPinSignalMapItemEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  ComponentPinSignalMapItem& mItem;

  std::optional<Uuid> mOldSignalUuid;
  std::optional<Uuid> mNewSignalUuid;
  CmpSigPinDisplayType mOldDisplayType;
  CmpSigPinDisplayType mNewDisplayType;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
