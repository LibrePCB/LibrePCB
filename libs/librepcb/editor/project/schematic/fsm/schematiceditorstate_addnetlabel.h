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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDNETLABEL_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDNETLABEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;
class SI_NetLabel;
class Schematic;

namespace editor {

class CmdSchematicNetLabelEdit;

/*******************************************************************************
 *  Class SchematicEditorState_AddNetLabel
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_AddNetLabel class
 */
class SchematicEditorState_AddNetLabel final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_AddNetLabel() = delete;
  SchematicEditorState_AddNetLabel(
      const SchematicEditorState_AddNetLabel& other) = delete;
  explicit SchematicEditorState_AddNetLabel(const Context& context) noexcept;
  virtual ~SchematicEditorState_AddNetLabel() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToSchematicPage(int index) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;
  virtual bool processMirror(Qt::Orientation orientation) noexcept override;

  // Operator Overloadings
  SchematicEditorState_AddNetLabel& operator=(
      const SchematicEditorState_AddNetLabel& rhs) = delete;

private:  // Methods
  bool addLabel(const Point& pos) noexcept;
  bool updateLabel(const Point& pos) noexcept;
  bool fixLabel(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;

private:  // Data
  bool mUndoCmdActive;
  SI_NetLabel* mCurrentNetLabel;
  CmdSchematicNetLabelEdit* mEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
