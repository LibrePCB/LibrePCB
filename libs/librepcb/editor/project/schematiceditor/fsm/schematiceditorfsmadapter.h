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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORFSMADAPTER_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORFSMADAPTER_H

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

class NetSignal;
class Schematic;

namespace editor {

class SchematicEditorState;
class SchematicGraphicsScene;

/*******************************************************************************
 *  Class SchematicEditorFsmAdapter
 ******************************************************************************/

/**
 * @brief Interface for the integration of the schematic editor FSM
 */
class SchematicEditorFsmAdapter {
public:
  virtual Schematic* fsmGetActiveSchematic() noexcept = 0;
  virtual SchematicGraphicsScene* fsmGetGraphicsScene() noexcept = 0;
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
  virtual Point fsmMapGlobalPosToScenePos(const QPoint& pos, bool boundToView,
                                          bool mapToGrid) const noexcept = 0;
  virtual void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept = 0;
  virtual void fsmAbortBlockingToolsInOtherEditors() noexcept = 0;
  virtual void fsmSetStatusBarMessage(const QString& message,
                                      int timeoutMs = -1) noexcept = 0;

  enum class Tool {
    None,
    Select,
    Wire,
    NetLabel,
    Polygon,
    Text,
    Component,
    Measure,
  };
  virtual void fsmSetTool(Tool tool, SchematicEditorState* state) noexcept = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
