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

#ifndef LIBREPCB_EDITOR_CMDSYMBOLPINEDIT_H
#define LIBREPCB_EDITOR_CMDSYMBOLPINEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../cmd/cmdlistelementinsert.h"
#include "../../cmd/cmdlistelementremove.h"
#include "../../cmd/cmdlistelementsswap.h"
#include "../../undocommand.h"

#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdSymbolPinEdit
 ******************************************************************************/

/**
 * @brief The CmdSymbolPinEdit class
 */
class CmdSymbolPinEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdSymbolPinEdit() = delete;
  CmdSymbolPinEdit(const CmdSymbolPinEdit& other) = delete;
  explicit CmdSymbolPinEdit(std::shared_ptr<SymbolPin> pin) noexcept;
  ~CmdSymbolPinEdit() noexcept;

  // Setters
  void setName(const CircuitIdentifier& name, bool immediate) noexcept;
  void setLength(const UnsignedLength& length, bool immediate) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirror(Qt::Orientation orientation, const Point& center,
              bool immediate) noexcept;

  // Operator Overloadings
  CmdSymbolPinEdit& operator=(const CmdSymbolPinEdit& rhs) = delete;

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
  std::shared_ptr<SymbolPin> mPin;

  // General Attributes
  CircuitIdentifier mOldName;
  CircuitIdentifier mNewName;
  UnsignedLength mOldLength;
  UnsignedLength mNewLength;
  Point mOldPos;
  Point mNewPos;
  Angle mOldRotation;
  Angle mNewRotation;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdSymbolPinInsert =
    CmdListElementInsert<SymbolPin, SymbolPinListNameProvider,
                         SymbolPin::Event>;
using CmdSymbolPinRemove =
    CmdListElementRemove<SymbolPin, SymbolPinListNameProvider,
                         SymbolPin::Event>;
using CmdSymbolPinsSwap =
    CmdListElementsSwap<SymbolPin, SymbolPinListNameProvider, SymbolPin::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
