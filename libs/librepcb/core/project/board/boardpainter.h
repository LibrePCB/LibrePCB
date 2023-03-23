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

#ifndef LIBREPCB_CORE_BOARDPAINTER_H
#define LIBREPCB_CORE_BOARDPAINTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../export/graphicsexport.h"
#include "../../geometry/hole.h"
#include "../../library/pkg/footprintpad.h"
#include "../../types/length.h"
#include "../../utils/transform.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Circle;
class Path;
class Polygon;
class StrokeFont;
class StrokeText;
class Text;
class Via;

/*******************************************************************************
 *  Class BoardPainter
 ******************************************************************************/

/**
 * @brief Paints a ::librepcb::Board to a QPainter
 *
 * Used for ::librepcb::GraphicsExport.
 */
class BoardPainter final : public GraphicsPagePainter {
  struct Trace {
    const Layer* layer;
    Point startPosition;
    Point endPosition;
    PositiveLength width;
  };

  struct Pad {
    Transform transform;
    QList<std::pair<const Layer*, PadGeometry>> layerGeometries;
    QList<PadHole> holes;
  };

  struct Footprint {
    Transform transform;
    QList<Pad> pads;
    QList<Polygon> polygons;
    QList<Circle> circles;
    QList<Hole> holes;  ///< Important: Stop mask set to explicit value!
  };

  struct Plane {
    const Layer* layer;
    QVector<Path> fragments;
  };

  struct ColorContent {
    QList<QPainterPath> areas;
    QList<QPainterPath> thtPadAreas;  ///< Drawn on Theme::Color::sBoardPads
    QList<Trace> traces;
    QList<Polygon> polygons;
    QList<Circle> circles;
    QList<Hole> holes;
    QList<Hole> padHoles;
    QList<Text> texts;
  };

public:
  // Constructors / Destructor
  BoardPainter() = delete;
  explicit BoardPainter(const Board& board);
  BoardPainter(const BoardPainter& other) = delete;
  ~BoardPainter() noexcept;

  // General Methods
  void paint(QPainter& painter, const GraphicsExportSettings& settings) const
      noexcept override;

  // Operator Overloadings
  BoardPainter& operator=(const BoardPainter& rhs) = delete;

private:  // Methods
  void initContentByColor() const noexcept;

private:  // Data
  const StrokeFont& mStrokeFont;

  QList<Footprint> mFootprints;
  QList<Via> mVias;
  QList<Trace> mTraces;
  QList<Plane> mPlanes;
  QList<Polygon> mPolygons;
  QList<StrokeText> mStrokeTexts;
  QList<Hole> mHoles;  ///< Important: Stop mask set to explicit value!

  mutable QMutex mMutex;
  mutable QHash<QString, ColorContent> mContentByColor;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
