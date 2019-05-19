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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <librepcb/common/graphics/graphicslayername.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Polygon;
class PolygonGraphicsItem;
class CmdPolygonEdit;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class SymbolEditorState_DrawPolygonBase
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_DrawPolygonBase class
 */
class SymbolEditorState_DrawPolygonBase : public SymbolEditorState {
  Q_OBJECT

public:
  // Types
  enum class Mode { LINE, RECT, POLYGON };

  // Constructors / Destructor
  SymbolEditorState_DrawPolygonBase() = delete;
  SymbolEditorState_DrawPolygonBase(
      const SymbolEditorState_DrawPolygonBase& other) = delete;
  SymbolEditorState_DrawPolygonBase(const Context& context, Mode mode) noexcept;
  virtual ~SymbolEditorState_DrawPolygonBase() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  SymbolEditorState_DrawPolygonBase& operator       =(
      const SymbolEditorState_DrawPolygonBase& rhs) = delete;

private:  // Methods
  bool start(const Point& pos) noexcept;
  bool abort() noexcept;
  bool addNextSegment(const Point& pos) noexcept;
  bool updateCurrentPosition(const Point& pos) noexcept;

  void layerComboBoxValueChanged(const QString& layerName) noexcept;
  void lineWidthSpinBoxValueChanged(double value) noexcept;
  void angleSpinBoxValueChanged(double value) noexcept;
  void fillCheckBoxCheckedChanged(bool checked) noexcept;
  void grabAreaCheckBoxCheckedChanged(bool checked) noexcept;

private:  // Types / Data
  Mode                           mMode;
  QScopedPointer<CmdPolygonEdit> mEditCmd;
  std::shared_ptr<Polygon>       mCurrentPolygon;
  Point                          mSegmentStartPos;
  PolygonGraphicsItem*           mCurrentGraphicsItem;

  // parameter memory
  GraphicsLayerName mLastLayerName;
  UnsignedLength    mLastLineWidth;
  Angle             mLastAngle;
  bool              mLastFill;
  bool              mLastGrabArea;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H
