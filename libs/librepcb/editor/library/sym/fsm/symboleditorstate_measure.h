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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_MEASURE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_MEASURE_H

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
namespace editor {

class MeasureTool;

/*******************************************************************************
 *  Class SymbolEditorState_Measure
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_Measure class
 */
class SymbolEditorState_Measure final : public SymbolEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_Measure() = delete;
  SymbolEditorState_Measure(const SymbolEditorState_Measure& other) = delete;
  explicit SymbolEditorState_Measure(const Context& context) noexcept;
  ~SymbolEditorState_Measure() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;
  QSet<EditorWidgetBase::Feature> getAvailableFeatures() const
      noexcept override;

  // Event Handlers
  bool processKeyPressed(const QKeyEvent& e) noexcept override;
  bool processKeyReleased(const QKeyEvent& e) noexcept override;
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processCopy() noexcept override;
  bool processRemove() noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  SymbolEditorState_Measure& operator=(const SymbolEditorState_Measure& rhs) =
      delete;

private:  // Data
  QScopedPointer<MeasureTool> mTool;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
