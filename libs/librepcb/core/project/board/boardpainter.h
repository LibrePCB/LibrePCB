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
#include "../../graphics/graphicslayername.h"
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
class Hole;
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
    QString layerName;
    Point startPosition;
    Point endPosition;
    PositiveLength width;
  };

  struct Pad {
    Transform transform;
    QList<std::pair<QString, PadGeometry>> layerGeometries;
    QList<Hole> holes;
  };

  struct Footprint {
    Transform transform;
    QList<Pad> pads;
    QList<Polygon> polygons;
    QList<Circle> circles;
    QList<Hole> holes;
  };

  struct Plane {
    QString layerName;
    QVector<Path> fragments;
  };

  struct LayerContent {
    QList<QPainterPath> areas;
    QList<QPainterPath> thtPadAreas;  ///< Drawn on GraphicsLayer::sBoardPadsTht
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
  void initContentByLayer() const noexcept;

private:  // Data
  const StrokeFont& mStrokeFont;

  QList<Footprint> mFootprints;
  QList<Via> mVias;
  QList<Trace> mTraces;
  QList<Plane> mPlanes;
  QList<Polygon> mPolygons;
  QList<StrokeText> mStrokeTexts;
  QList<Hole> mHoles;

  mutable QMutex mMutex;
  mutable QHash<QString, LayerContent> mContentByLayer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
