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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_DRAWPOLYGON_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_DRAWPOLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/project/board/boardpolygondata.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Polygon;

namespace editor {

class CmdBoardPolygonEdit;

/*******************************************************************************
 *  Class BoardEditorState_DrawPolygon
 ******************************************************************************/

/**
 * @brief The "draw polygon" state/tool of the board editor
 */
class BoardEditorState_DrawPolygon final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_DrawPolygon() = delete;
  BoardEditorState_DrawPolygon(const BoardEditorState_DrawPolygon& other) =
      delete;
  explicit BoardEditorState_DrawPolygon(const Context& context) noexcept;
  virtual ~BoardEditorState_DrawPolygon() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() noexcept;
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

  // Operator Overloadings
  BoardEditorState_DrawPolygon& operator=(
      const BoardEditorState_DrawPolygon& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void lineWidthChanged(const UnsignedLength& width);
  void filledChanged(bool filled);

private:  // Methods
  bool startAddPolygon(const Point& pos) noexcept;
  bool addSegment(const Point& pos) noexcept;
  bool updateLastVertexPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;

private:
  // State
  bool mIsUndoCmdActive;
  Point mLastSegmentPos;

  // Current tool settings
  BoardPolygonData mCurrentProperties;

  // Information about the current polygon to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Polygon* mCurrentPolygon;
  std::unique_ptr<CmdBoardPolygonEdit> mCurrentPolygonEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
