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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWPOLYGONBASE_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWPOLYGONBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/core/graphics/graphicslayername.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Polygon;
class PolygonGraphicsItem;

namespace editor {

class CmdPolygonEdit;

/*******************************************************************************
 *  Class PackageEditorState_DrawPolygonBase
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawPolygonBase class
 */
class PackageEditorState_DrawPolygonBase : public PackageEditorState {
  Q_OBJECT

public:
  // Types
  enum class Mode { LINE, RECT, POLYGON };

  // Constructors / Destructor
  PackageEditorState_DrawPolygonBase() = delete;
  PackageEditorState_DrawPolygonBase(
      const PackageEditorState_DrawPolygonBase& other) = delete;
  PackageEditorState_DrawPolygonBase(Context& context, Mode mode) noexcept;
  virtual ~PackageEditorState_DrawPolygonBase() noexcept;

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
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  PackageEditorState_DrawPolygonBase& operator=(
      const PackageEditorState_DrawPolygonBase& rhs) = delete;

private:  // Methods
  bool start() noexcept;
  bool abort(bool showErrMsgBox = true) noexcept;
  bool addNextSegment() noexcept;
  void updateCursorPosition(Qt::KeyboardModifiers modifiers) noexcept;
  void updatePolygonPath() noexcept;

  void layerComboBoxValueChanged(const GraphicsLayerName& layerName) noexcept;
  void lineWidthEditValueChanged(const UnsignedLength& value) noexcept;
  void angleEditValueChanged(const Angle& value) noexcept;
  void fillCheckBoxCheckedChanged(bool checked) noexcept;
  void grabAreaCheckBoxCheckedChanged(bool checked) noexcept;

private:  // Types / Data
  Mode mMode;
  bool mIsUndoCmdActive;
  QScopedPointer<CmdPolygonEdit> mEditCmd;
  std::shared_ptr<Polygon> mCurrentPolygon;
  std::shared_ptr<PolygonGraphicsItem> mCurrentGraphicsItem;
  Point mLastScenePos;
  Point mCursorPos;

  // parameter memory
  GraphicsLayerName mLastLayerName;
  UnsignedLength mLastLineWidth;
  Angle mLastAngle;
  bool mLastFill;
  bool mLastGrabArea;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
