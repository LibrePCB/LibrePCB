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

#ifndef LIBREPCB_LIBRARY_CMDCOMPONENTSYMBOLVARIANTEDIT_H
#define LIBREPCB_LIBRARY_CMDCOMPONENTSYMBOLVARIANTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../componentsymbolvariant.h"

#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class CmdComponentSymbolVariantEdit
 ******************************************************************************/

/**
 * @brief The CmdComponentSymbolVariantEdit class
 */
class CmdComponentSymbolVariantEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdComponentSymbolVariantEdit() = delete;
  CmdComponentSymbolVariantEdit(const CmdComponentSymbolVariantEdit& other) =
      delete;
  explicit CmdComponentSymbolVariantEdit(
      ComponentSymbolVariant& variant) noexcept;
  ~CmdComponentSymbolVariantEdit() noexcept;

  // Setters
  void setNorm(const QString& norm) noexcept;
  void setNames(const LocalizedNameMap& names) noexcept;
  void setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept;
  void setSymbolItems(const ComponentSymbolVariantItemList& items) noexcept;

  // Operator Overloadings
  CmdComponentSymbolVariantEdit& operator=(
      const CmdComponentSymbolVariantEdit& rhs) = delete;

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  ComponentSymbolVariant& mVariant;

  QString mOldNorm;
  QString mNewNorm;
  LocalizedNameMap mOldNames;
  LocalizedNameMap mNewNames;
  LocalizedDescriptionMap mOldDescriptions;
  LocalizedDescriptionMap mNewDescriptions;
  ComponentSymbolVariantItemList mOldSymbolItems;
  ComponentSymbolVariantItemList mNewSymbolItems;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdComponentSymbolVariantInsert =
    CmdListElementInsert<ComponentSymbolVariant,
                         ComponentSymbolVariantListNameProvider,
                         ComponentSymbolVariant::Event>;
using CmdComponentSymbolVariantRemove =
    CmdListElementRemove<ComponentSymbolVariant,
                         ComponentSymbolVariantListNameProvider,
                         ComponentSymbolVariant::Event>;
using CmdComponentSymbolVariantsSwap =
    CmdListElementsSwap<ComponentSymbolVariant,
                        ComponentSymbolVariantListNameProvider,
                        ComponentSymbolVariant::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif
