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

#ifndef LIBREPCB_EDITOR_BOARDEDITORFSMADAPTER_H
#define LIBREPCB_EDITOR_BOARDEDITORFSMADAPTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class NetSignal;

namespace editor {

class BoardEditorState;
class BoardEditorState_AddDevice;
class BoardEditorState_AddHole;
class BoardEditorState_AddPad;
class BoardEditorState_AddStrokeText;
class BoardEditorState_AddVia;
class BoardEditorState_DrawPlane;
class BoardEditorState_DrawPolygon;
class BoardEditorState_DrawTrace;
class BoardEditorState_DrawZone;
class BoardEditorState_Measure;
class BoardEditorState_Select;
class BoardGraphicsScene;

/*******************************************************************************
 *  Class BoardEditorFsmAdapter
 ******************************************************************************/

/**
 * @brief Interface for the integration of the board editor FSM
 */
class BoardEditorFsmAdapter {
public:
  enum class Feature : quint32 {
    Select = (1 << 0),
    Cut = (1 << 1),
    Copy = (1 << 2),
    Paste = (1 << 3),
    Remove = (1 << 4),
    Rotate = (1 << 5),
    Flip = (1 << 6),
    // MoveAlign = (1 << 7),
    SnapToGrid = (1 << 8),
    ResetTexts = (1 << 9),
    Lock = (1 << 10),
    Unlock = (1 << 11),
    Properties = (1 << 12),
    ModifyLineWidth = (1 << 13),
    ImportGraphics = (1 << 14),
  };
  Q_DECLARE_FLAGS(Features, Feature)

  virtual BoardGraphicsScene* fsmGetGraphicsScene() noexcept = 0;
  virtual bool fsmGetIgnoreLocks() const noexcept = 0;
  virtual void fsmSetViewCursor(
      const std::optional<Qt::CursorShape>& shape) noexcept = 0;
  virtual void fsmSetViewGrayOut(bool grayOut) noexcept = 0;
  virtual void fsmSetViewInfoBoxText(const QString& text) noexcept = 0;
  virtual void fsmSetViewRuler(
      const std::optional<std::pair<Point, Point>>& pos) noexcept = 0;
  virtual void fsmSetSceneCursor(const Point& pos, bool cross,
                                 bool circle) noexcept = 0;
  virtual QPainterPath fsmCalcPosWithTolerance(
      const Point& pos, qreal multiplier) const noexcept = 0;
  virtual Point fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept = 0;
  virtual void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept = 0;
  virtual void fsmAbortBlockingToolsInOtherEditors() noexcept = 0;
  virtual void fsmSetStatusBarMessage(const QString& message,
                                      int timeoutMs = -1) noexcept = 0;
  virtual void fsmSetFeatures(Features features) noexcept = 0;

  virtual void fsmToolLeave() noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_Select& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_DrawTrace& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_AddVia& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_AddPad& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_DrawPolygon& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_AddStrokeText& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_DrawPlane& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_DrawZone& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_AddHole& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_AddDevice& state) noexcept = 0;
  virtual void fsmToolEnter(BoardEditorState_Measure& state) noexcept = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::BoardEditorFsmAdapter::Feature)
Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::BoardEditorFsmAdapter::Features)

#endif
