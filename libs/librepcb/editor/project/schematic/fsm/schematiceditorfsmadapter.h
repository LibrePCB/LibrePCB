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
class SchematicEditorState_AddComponent;
class SchematicEditorState_AddImage;
class SchematicEditorState_AddNetLabel;
class SchematicEditorState_AddText;
class SchematicEditorState_DrawPolygon;
class SchematicEditorState_DrawWire;
class SchematicEditorState_Measure;
class SchematicEditorState_Select;
class SchematicGraphicsScene;

/*******************************************************************************
 *  Class SchematicEditorFsmAdapter
 ******************************************************************************/

/**
 * @brief Interface for the integration of the schematic editor FSM
 */
class SchematicEditorFsmAdapter {
public:
  enum class Feature : quint32 {
    Select = (1 << 0),
    Cut = (1 << 1),
    Copy = (1 << 2),
    Paste = (1 << 3),
    Remove = (1 << 4),
    Rotate = (1 << 5),
    Mirror = (1 << 6),
    SnapToGrid = (1 << 7),
    ResetTexts = (1 << 8),
    Properties = (1 << 9),
  };
  Q_DECLARE_FLAGS(Features, Feature)

  virtual SchematicGraphicsScene* fsmGetGraphicsScene() noexcept = 0;
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
  virtual void fsmZoomToSceneRect(const QRectF& r) noexcept = 0;
  virtual void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept = 0;
  virtual void fsmAbortBlockingToolsInOtherEditors() noexcept = 0;
  virtual void fsmSetStatusBarMessage(const QString& message,
                                      int timeoutMs = -1) noexcept = 0;
  virtual void fsmSetFeatures(Features features) noexcept = 0;

  virtual void fsmToolLeave() noexcept = 0;
  virtual void fsmToolEnter(SchematicEditorState_Select& state) noexcept = 0;
  virtual void fsmToolEnter(SchematicEditorState_DrawWire& state) noexcept = 0;
  virtual void fsmToolEnter(
      SchematicEditorState_AddNetLabel& state) noexcept = 0;
  virtual void fsmToolEnter(
      SchematicEditorState_AddComponent& state) noexcept = 0;
  virtual void fsmToolEnter(
      SchematicEditorState_DrawPolygon& state) noexcept = 0;
  virtual void fsmToolEnter(SchematicEditorState_AddText& state) noexcept = 0;
  virtual void fsmToolEnter(SchematicEditorState_AddImage& state) noexcept = 0;
  virtual void fsmToolEnter(SchematicEditorState_Measure& state) noexcept = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::SchematicEditorFsmAdapter::Feature)
Q_DECLARE_OPERATORS_FOR_FLAGS(
    librepcb::editor::SchematicEditorFsmAdapter::Features)

#endif
