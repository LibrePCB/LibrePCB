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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDLABEL_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDLABEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/core/types/angle.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;
class SI_BusLabel;
class SI_NetLabel;
class Schematic;

namespace editor {

class CmdSchematicBusLabelEdit;
class CmdSchematicNetLabelEdit;

/*******************************************************************************
 *  Class SchematicEditorState_AddLabel
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_AddLabel class
 */
class SchematicEditorState_AddLabel final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_AddLabel() = delete;
  SchematicEditorState_AddLabel(const SchematicEditorState_AddLabel& other) =
      delete;
  explicit SchematicEditorState_AddLabel(const Context& context) noexcept;
  ~SchematicEditorState_AddLabel() noexcept override;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processRotate(const Angle& rotation) noexcept override;
  bool processMirror(Qt::Orientation orientation) noexcept override;
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Connection to UI
  Angle getRotation() const noexcept;
  void setRotation(const Angle& rotation) noexcept;
  bool getMirrored() const noexcept;
  void setMirrored(bool mirrored) noexcept;
  bool getAutoOrientation() const noexcept { return mAutoOrientation; }
  void setAutoOrientation(bool enable) noexcept;

  // Operator Overloadings
  SchematicEditorState_AddLabel& operator=(
      const SchematicEditorState_AddLabel& rhs) = delete;

signals:
  void rotationChanged(const Angle& rotation);
  void mirroredChanged(bool mirrored);
  void autoOrientationChanged(bool enabled);

private:  // Methods
  bool addLabel(const Point& pos) noexcept;
  bool updateLabel(const Point& pos) noexcept;
  bool fixLabel(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void updateLabelAutoOrientation() noexcept;
  std::optional<std::pair<Angle, bool>> calcAutoOrientation(
      const Point& pos, const QVector<std::pair<Point, Point>>& lines) noexcept;
  bool storeRotation(const Angle& rotation) noexcept;
  bool storeMirrored(bool mirrored) noexcept;
  bool storeAutoOrientation(bool enable) noexcept;

private:  // Data
  bool mUndoCmdActive;

  // Current tool settings
  Angle mCurrentRotation;
  bool mCurrentMirrored;
  bool mAutoOrientation;

  // Information about the current label to place
  SI_NetLabel* mCurrentNetLabel;
  CmdSchematicNetLabelEdit* mNetLabelEditCmd;
  SI_BusLabel* mCurrentBusLabel;
  CmdSchematicBusLabelEdit* mBusLabelEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
