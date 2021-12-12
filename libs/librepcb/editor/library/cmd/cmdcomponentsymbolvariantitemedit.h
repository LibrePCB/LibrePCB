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

#ifndef LIBREPCB_EDITOR_CMDCOMPONENTSYMBOLVARIANTITEMEDIT_H
#define LIBREPCB_EDITOR_CMDCOMPONENTSYMBOLVARIANTITEMEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/cmp/componentsymbolvariant.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdComponentSymbolVariantItemEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentSymbolVariantItemEdit class
 */
class CmdComponentSymbolVariantItemEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdComponentSymbolVariantItemEdit() = delete;
  CmdComponentSymbolVariantItemEdit(
      const CmdComponentSymbolVariantItemEdit& other) = delete;
  explicit CmdComponentSymbolVariantItemEdit(
      ComponentSymbolVariantItem& item) noexcept;
  ~CmdComponentSymbolVariantItemEdit() noexcept;

  // Setters
  void setSymbolUuid(const Uuid& uuid) noexcept;
  void setSymbolPosition(const Point& pos) noexcept;
  void setSymbolRotation(const Angle& rot) noexcept;
  void setIsRequired(bool required) noexcept;
  void setSuffix(const ComponentSymbolVariantItemSuffix& suffix) noexcept;
  void setPinSignalMap(const ComponentPinSignalMap& map) noexcept;

  // Operator Overloadings
  CmdComponentSymbolVariantItemEdit& operator=(
      const CmdComponentSymbolVariantItemEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  ComponentSymbolVariantItem& mItem;

  Uuid mOldSymbolUuid;
  Uuid mNewSymbolUuid;
  Point mOldSymbolPos;
  Point mNewSymbolPos;
  Angle mOldSymbolRot;
  Angle mNewSymbolRot;
  bool mOldIsRequired;
  bool mNewIsRequired;
  ComponentSymbolVariantItemSuffix mOldSuffix;
  ComponentSymbolVariantItemSuffix mNewSuffix;
  ComponentPinSignalMap mOldPinSignalMap;
  ComponentPinSignalMap mNewPinSignalMap;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdComponentSymbolVariantItemInsert =
    CmdListElementInsert<ComponentSymbolVariantItem,
                         ComponentSymbolVariantItemListNameProvider,
                         ComponentSymbolVariantItem::Event>;
using CmdComponentSymbolVariantItemRemove =
    CmdListElementRemove<ComponentSymbolVariantItem,
                         ComponentSymbolVariantItemListNameProvider,
                         ComponentSymbolVariantItem::Event>;
using CmdComponentSymbolVariantItemsSwap =
    CmdListElementsSwap<ComponentSymbolVariantItem,
                        ComponentSymbolVariantItemListNameProvider,
                        ComponentSymbolVariantItem::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
