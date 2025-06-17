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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGONBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <librepcb/core/geometry/polygon.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CmdPolygonEdit;
class PolygonGraphicsItem;

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
  enum class Mode { LINE, ARC, RECT, POLYGON };

  // Constructors / Destructor
  SymbolEditorState_DrawPolygonBase() = delete;
  SymbolEditorState_DrawPolygonBase(
      const SymbolEditorState_DrawPolygonBase& other) = delete;
  SymbolEditorState_DrawPolygonBase(const Context& context, Mode mode) noexcept;
  virtual ~SymbolEditorState_DrawPolygonBase() noexcept;

  // General Methods
  bool processKeyPressed(const GraphicsSceneKeyEvent& e) noexcept override;
  bool processKeyReleased(const GraphicsSceneKeyEvent& e) noexcept override;
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processAbortCommand() noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() const noexcept;
  const Layer& getLayer() const noexcept {
    return mCurrentProperties.getLayer();
  }
  void setLayer(const Layer& layer) noexcept;
  const UnsignedLength& getLineWidth() const noexcept {
    return mCurrentProperties.getLineWidth();
  }
  void setLineWidth(const UnsignedLength& width) noexcept;
  bool getFilled() const noexcept { return mCurrentProperties.isFilled(); }
  void setFilled(bool filled) noexcept;
  bool getGrabArea() const noexcept { return mCurrentProperties.isGrabArea(); }
  void setGrabArea(bool grabArea) noexcept;
  const Angle& getAngle() const noexcept { return mLastAngle; }
  void setAngle(const Angle& angle) noexcept;

  // Operator Overloadings
  SymbolEditorState_DrawPolygonBase& operator=(
      const SymbolEditorState_DrawPolygonBase& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void lineWidthChanged(const UnsignedLength& width);
  void filledChanged(bool filled);
  void grabAreaChanged(bool grabArea);
  void angleChanged(const Angle& angle);

protected:
  virtual void notifyToolEnter() noexcept = 0;

private:  // Methods
  bool start() noexcept;
  bool abort(bool showErrMsgBox = true) noexcept;
  bool addNextSegment() noexcept;
  void updateCursorPosition(Qt::KeyboardModifiers modifiers) noexcept;
  void updatePolygonPath() noexcept;
  void updateOverlayText() noexcept;
  void updateStatusBarMessage() noexcept;

private:
  const Mode mMode;
  Point mLastScenePos;
  Angle mLastAngle;
  Point mCursorPos;
  bool mIsUndoCmdActive;

  // Arc tool state
  Point mArcCenter;
  bool mArcInSecondState;

  // Current tool settings
  Polygon mCurrentProperties;

  // Information about the current polygon to place. Only valid if
  // mIsUndoCmdActive == true.
  std::shared_ptr<Polygon> mCurrentPolygon;
  std::unique_ptr<CmdPolygonEdit> mCurrentEditCmd;
  std::shared_ptr<PolygonGraphicsItem> mCurrentGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
