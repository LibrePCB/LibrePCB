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

#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
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

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processRotate(const Angle& rotation) noexcept override;
  bool processMirror(Qt::Orientation orientation) noexcept override;
  bool processImportPins() noexcept override;

  // Connection to UI
  const CircuitIdentifier& getName() const noexcept {
    return mCurrentProperties.getName();
  }
  void setName(const CircuitIdentifier& name) noexcept;
  const UnsignedLength& getLength() const noexcept {
    return mCurrentProperties.getLength();
  }
  void setLength(const UnsignedLength& length) noexcept;

  // Operator Overloadings
  SymbolEditorState_AddPins& operator=(const SymbolEditorState_AddPins& rhs) =
      delete;

signals:
  void nameChanged(const CircuitIdentifier& name);
  void lengthChanged(const UnsignedLength& length);

private:  // Methods
  bool addNextPin(const Point& pos) noexcept;
  CircuitIdentifier determineNextPinName() const noexcept;
  bool hasPin(const QString& name) const noexcept;

private:
  SymbolPin mCurrentProperties;

  std::shared_ptr<SymbolPin> mCurrentPin;
  std::shared_ptr<SymbolPinGraphicsItem> mCurrentGraphicsItem;
  std::unique_ptr<CmdSymbolPinEdit> mCurrentEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
