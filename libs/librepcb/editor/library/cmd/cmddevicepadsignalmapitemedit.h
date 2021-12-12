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

#ifndef LIBREPCB_EDITOR_CMDDEVICEPADSIGNALMAPITEMEDIT_H
#define LIBREPCB_EDITOR_CMDDEVICEPADSIGNALMAPITEMEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/dev/devicepadsignalmap.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdDevicePadSignalMapItemEdit
 ******************************************************************************/

/**
 * @brief The CmdDevicePadSignalMapItemEdit class
 */
class CmdDevicePadSignalMapItemEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdDevicePadSignalMapItemEdit() = delete;
  CmdDevicePadSignalMapItemEdit(const CmdDevicePadSignalMapItemEdit& other) =
      delete;
  explicit CmdDevicePadSignalMapItemEdit(DevicePadSignalMapItem& item) noexcept;
  ~CmdDevicePadSignalMapItemEdit() noexcept;

  // Setters
  void setSignalUuid(const tl::optional<Uuid>& uuid) noexcept;

  // Operator Overloadings
  CmdDevicePadSignalMapItemEdit& operator=(
      const CmdDevicePadSignalMapItemEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  DevicePadSignalMapItem& mItem;

  tl::optional<Uuid> mOldSignalUuid;
  tl::optional<Uuid> mNewSignalUuid;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdDevicePadSignalMapItemInsert =
    CmdListElementInsert<DevicePadSignalMapItem, DevicePadSignalMapNameProvider,
                         DevicePadSignalMapItem::Event>;
using CmdDevicePadSignalMapItemRemove =
    CmdListElementRemove<DevicePadSignalMapItem, DevicePadSignalMapNameProvider,
                         DevicePadSignalMapItem::Event>;
using CmdDevicePadSignalMapItemsSwap =
    CmdListElementsSwap<DevicePadSignalMapItem, DevicePadSignalMapNameProvider,
                        DevicePadSignalMapItem::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
