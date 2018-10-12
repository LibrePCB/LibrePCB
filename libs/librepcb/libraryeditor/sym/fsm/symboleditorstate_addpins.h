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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_ADDPINS_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_ADDPINS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class SymbolPin;
class SymbolPinGraphicsItem;
class CmdSymbolPinEdit;

namespace editor {

/*******************************************************************************
 *  Class SymbolEditorState_AddPins
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_AddPins class
 *
 * @author  ubruhin
 * @date    2016-11-27
 */
class SymbolEditorState_AddPins final : public SymbolEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_AddPins()                                       = delete;
  SymbolEditorState_AddPins(const SymbolEditorState_AddPins& other) = delete;
  explicit SymbolEditorState_AddPins(const Context& context) noexcept;
  ~SymbolEditorState_AddPins() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processRotateCw() noexcept override;
  bool processRotateCcw() noexcept override;

  // Operator Overloadings
  SymbolEditorState_AddPins& operator=(const SymbolEditorState_AddPins& rhs) =
      delete;

private:  // Methods
  bool    addNextPin(const Point& pos, const Angle& rot) noexcept;
  void    nameLineEditTextChanged(const QString& text) noexcept;
  void    lengthSpinBoxValueChanged(double value) noexcept;
  QString determineNextPinName() const noexcept;
  bool    hasPin(const QString& name) const noexcept;

private:  // Types / Data
  QScopedPointer<CmdSymbolPinEdit> mEditCmd;
  SymbolPin*                       mCurrentPin;
  SymbolPinGraphicsItem*           mCurrentGraphicsItem;
  QLineEdit*                       mNameLineEdit;

  // parameter memory
  UnsignedLength mLastLength;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_ADDPINS_H
