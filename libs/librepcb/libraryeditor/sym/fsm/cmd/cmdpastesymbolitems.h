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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDPASTESYMBOLITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDPASTESYMBOLITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/point.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class Symbol;
class SymbolGraphicsItem;

namespace editor {

class SymbolClipboardData;

/*******************************************************************************
 *  Class CmdPasteSymbolItems
 ******************************************************************************/

/**
 * @brief The CmdPasteSymbolItems class
 */
class CmdPasteSymbolItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdPasteSymbolItems()                                 = delete;
  CmdPasteSymbolItems(const CmdPasteSymbolItems& other) = delete;
  CmdPasteSymbolItems(Symbol& symbol, SymbolGraphicsItem& graphicsItem,
                      std::unique_ptr<SymbolClipboardData> data,
                      const Point&                         posOffset) noexcept;
  ~CmdPasteSymbolItems() noexcept;

  // Operator Overloadings
  CmdPasteSymbolItems& operator=(const CmdPasteSymbolItems& rhs) = delete;

protected:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

private:  // Data
  Symbol&                              mSymbol;
  SymbolGraphicsItem&                  mGraphicsItem;
  std::unique_ptr<SymbolClipboardData> mData;
  Point                                mPosOffset;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDPASTESYMBOLITEMS_H
