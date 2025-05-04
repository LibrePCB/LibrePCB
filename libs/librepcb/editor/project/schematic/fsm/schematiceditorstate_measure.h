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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_MEASURE_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_MEASURE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class MeasureTool;

/*******************************************************************************
 *  Class SchematicEditorState_Measure
 ******************************************************************************/

/**
 * @brief The "measure" state/tool of the schematic editor
 */
class SchematicEditorState_Measure final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_Measure() = delete;
  SchematicEditorState_Measure(const SchematicEditorState_Measure& other) =
      delete;
  explicit SchematicEditorState_Measure(const Context& context) noexcept;
  virtual ~SchematicEditorState_Measure() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processCopy() noexcept override;
  virtual bool processRemove() noexcept override;
  virtual bool processAbortCommand() noexcept override;
  virtual bool processKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept override;
  virtual bool processKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToSchematicPage(int index) noexcept override;
  virtual void processSwitchedSchematicPage() noexcept override;

  // Operator Overloadings
  SchematicEditorState_Measure& operator=(
      const SchematicEditorState_Measure& rhs) = delete;

private:  // Data
  QScopedPointer<MeasureTool> mTool;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
