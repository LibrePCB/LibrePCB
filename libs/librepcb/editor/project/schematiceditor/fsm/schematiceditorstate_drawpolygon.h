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

#ifndef LIBREPCB_PROJECTEDITOR_SCHEMATICEDITORSTATE_DRAWPOLYGON_H
#define LIBREPCB_PROJECTEDITOR_SCHEMATICEDITORSTATE_DRAWPOLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/common/geometry/polygon.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CmdPolygonEdit;
class GraphicsLayerComboBox;
class UnsignedLengthEdit;

namespace project {

class SI_Polygon;
class Schematic;

namespace editor {

/*******************************************************************************
 *  Class SchematicEditorState_DrawPolygon
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_DrawPolygon class
 */
class SchematicEditorState_DrawPolygon final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_DrawPolygon() = delete;
  SchematicEditorState_DrawPolygon(
      const SchematicEditorState_DrawPolygon& other) = delete;
  explicit SchematicEditorState_DrawPolygon(const Context& context) noexcept;
  virtual ~SchematicEditorState_DrawPolygon() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToSchematicPage(int index) noexcept override;

  // Operator Overloadings
  SchematicEditorState_DrawPolygon& operator=(
      const SchematicEditorState_DrawPolygon& rhs) = delete;

private:  // Methods
  bool startAddPolygon(Schematic& schematic, const Point& pos) noexcept;
  bool addSegment(const Point& pos) noexcept;
  bool updateLastVertexPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void layerComboBoxLayerChanged(const GraphicsLayerName& layerName) noexcept;
  void widthEditValueChanged(const UnsignedLength& value) noexcept;
  void filledCheckBoxCheckedChanged(bool checked) noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  Polygon mLastPolygonProperties;
  Point mLastSegmentPos;

  // Information about the current polygon to place. Only valid if
  // mIsUndoCmdActive == true.
  SI_Polygon* mCurrentPolygon;
  QScopedPointer<CmdPolygonEdit> mCurrentPolygonEditCmd;

  // Widgets for the command toolbar
  QList<QAction*> mActionSeparators;
  QScopedPointer<QLabel> mLayerLabel;
  QScopedPointer<GraphicsLayerComboBox> mLayerComboBox;
  QScopedPointer<QLabel> mWidthLabel;
  QScopedPointer<UnsignedLengthEdit> mWidthEdit;
  QScopedPointer<QLabel> mFillLabel;
  QScopedPointer<QCheckBox> mFillCheckBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
