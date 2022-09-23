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

#ifndef LIBREPCB_EDITOR_MEASURETOOL_H
#define LIBREPCB_EDITOR_MEASURETOOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/point.h>
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Footprint;
class Path;
class Schematic;
class Symbol;
class Transform;

namespace editor {

class GraphicsView;

/*******************************************************************************
 *  Class MeasureTool
 ******************************************************************************/

/**
 * @brief Measure tool providing the measure functionality for the editor states
 */
class MeasureTool final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  MeasureTool() = delete;
  MeasureTool(const MeasureTool& other) = delete;
  explicit MeasureTool(GraphicsView& view, QObject* parent = nullptr) noexcept;
  ~MeasureTool() noexcept;

  // General Methods
  void setSymbol(const Symbol* symbol) noexcept;
  void setFootprint(const Footprint* footprint) noexcept;
  void setSchematic(const Schematic* schematic) noexcept;
  void setBoard(const Board* board) noexcept;
  void enter() noexcept;
  void leave() noexcept;

  // Event Handlers
  bool processKeyPressed(const QKeyEvent& e) noexcept;
  bool processKeyReleased(const QKeyEvent& e) noexcept;
  bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processCopy() noexcept;
  bool processRemove() noexcept;
  bool processAbortCommand() noexcept;

  // Operator Overloadings
  MeasureTool& operator=(const MeasureTool& rhs) = delete;

signals:
  void statusBarMessageChanged(const QString& message, int timeoutMs = -1);

private:  // Methods
  static QSet<Point> snapCandidatesFromSymbol(
      const Symbol& symbol, const Transform& transform) noexcept;
  static QSet<Point> snapCandidatesFromFootprint(
      const Footprint& footprint, const Transform& transform) noexcept;
  static QSet<Point> snapCandidatesFromPath(const Path& path) noexcept;
  static QSet<Point> snapCandidatesFromCircle(const Point& center,
                                              const Length& diameter) noexcept;
  void updateCursorPosition(Qt::KeyboardModifiers modifiers) noexcept;
  void updateRulerPositions() noexcept;
  void updateStatusBarMessage() noexcept;

private:  // Data
  QPointer<GraphicsView> mView;
  QSet<Point> mSnapCandidates;
  Point mLastScenePos;
  Point mCursorPos;
  bool mCursorSnapped;
  tl::optional<Point> mStartPos;
  tl::optional<Point> mEndPos;
};

}  // namespace editor
}  // namespace librepcb

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
