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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_ADDPINS_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_ADDPINS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;
class SymbolPin;

namespace editor {

class CmdSymbolPinEdit;
class SymbolPinGraphicsItem;

/*******************************************************************************
 *  Class SymbolEditorState_AddPins
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_AddPins class
 */
class SymbolEditorState_AddPins final : public SymbolEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_AddPins() = delete;
  SymbolEditorState_AddPins(const SymbolEditorState_AddPins& other) = delete;
  explicit SymbolEditorState_AddPins(const Context& context) noexcept;
  ~SymbolEditorState_AddPins() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;
  QSet<EditorWidgetBase::Feature> getAvailableFeatures()
      const noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processRotate(const Angle& rotation) noexcept override;
  bool processMirror(Qt::Orientation orientation) noexcept override;

  // Operator Overloadings
  SymbolEditorState_AddPins& operator=(const SymbolEditorState_AddPins& rhs) =
      delete;

private:  // Methods
  bool addNextPin(const Point& pos) noexcept;
  void nameLineEditTextChanged(const QString& text) noexcept;
  void lengthEditValueChanged(const UnsignedLength& value) noexcept;
  void execMassImport() noexcept;
  QString determineNextPinName() const noexcept;
  bool hasPin(const QString& name) const noexcept;

private:  // Types / Data
  std::shared_ptr<SymbolPin> mCurrentPin;
  std::shared_ptr<SymbolPinGraphicsItem> mCurrentGraphicsItem;
  std::unique_ptr<CmdSymbolPinEdit> mEditCmd;
  QLineEdit* mNameLineEdit;

  // parameter memory
  Angle mLastRotation;
  UnsignedLength mLastLength;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
