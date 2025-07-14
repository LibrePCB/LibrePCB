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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORFSMADAPTER_H
#define LIBREPCB_EDITOR_PACKAGEEDITORFSMADAPTER_H

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
namespace editor {

class FootprintGraphicsItem;
class GraphicsScene;
class PackageEditorState;
class PackageEditorState_AddHoles;
class PackageEditorState_AddNames;
class PackageEditorState_AddPads;
class PackageEditorState_AddValues;
class PackageEditorState_DrawArc;
class PackageEditorState_DrawCircle;
class PackageEditorState_DrawLine;
class PackageEditorState_DrawPolygon;
class PackageEditorState_DrawRect;
class PackageEditorState_DrawText;
class PackageEditorState_DrawZone;
class PackageEditorState_Measure;
class PackageEditorState_ReNumberPads;
class PackageEditorState_Select;

/*******************************************************************************
 *  Class PackageEditorFsmAdapter
 ******************************************************************************/

/**
 * @brief Interface for the integration of the Package editor FSM
 */
class PackageEditorFsmAdapter {
public:
  enum class Feature : quint32 {
    Select = (1 << 0),
    Cut = (1 << 1),
    Copy = (1 << 2),
    Paste = (1 << 3),
    Remove = (1 << 4),
    Rotate = (1 << 5),
    Mirror = (1 << 6),
    Flip = (1 << 7),
    MoveAlign = (1 << 8),
    SnapToGrid = (1 << 9),
    Properties = (1 << 10),
    ImportGraphics = (1 << 11),
  };
  Q_DECLARE_FLAGS(Features, Feature)

  virtual GraphicsScene* fsmGetGraphicsScene() noexcept = 0;
  virtual PositiveLength fsmGetGridInterval() const noexcept = 0;
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
  virtual void fsmSetStatusBarMessage(const QString& message,
                                      int timeoutMs = -1) noexcept = 0;
  virtual void fsmSetFeatures(Features features) noexcept = 0;

  virtual void fsmToolLeave() noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_Select& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawLine& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawRect& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawPolygon& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawCircle& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawArc& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_AddNames& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_AddValues& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawText& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_AddPads& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_DrawZone& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_AddHoles& state) noexcept = 0;
  virtual void fsmToolEnter(
      PackageEditorState_ReNumberPads& state) noexcept = 0;
  virtual void fsmToolEnter(PackageEditorState_Measure& state) noexcept = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::PackageEditorFsmAdapter::Feature)
Q_DECLARE_OPERATORS_FOR_FLAGS(
    librepcb::editor::PackageEditorFsmAdapter::Features)

#endif
