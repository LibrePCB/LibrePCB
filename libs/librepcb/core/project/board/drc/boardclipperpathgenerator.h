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

#ifndef LIBREPCB_CORE_BOARDCLIPPERPATHGENERATOR_H
#define LIBREPCB_CORE_BOARDCLIPPERPATHGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/path.h"
#include "../../../types/length.h"
#include "../../../utils/transform.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_FootprintPad;
class BI_NetLine;
class BI_Plane;
class BI_StrokeText;
class BI_Via;
class Board;
class Circle;
class Hole;
class Layer;
class NetSignal;

/*******************************************************************************
 *  Class BoardClipperPathGenerator
 ******************************************************************************/

/**
 * @brief The BoardClipperPathGenerator class creates a Clipper path from
 *        a ::librepcb::Board
 */
class BoardClipperPathGenerator final {
public:
  // Constructors / Destructor
  explicit BoardClipperPathGenerator(
      Board& board, const PositiveLength& maxArcTolerance) noexcept;
  ~BoardClipperPathGenerator() noexcept;

  // Getters
  const ClipperLib::Paths& getPaths() const noexcept { return mPaths; }
  void takePathsTo(ClipperLib::Paths& out) noexcept;

  // General Methods
  void addCopper(const Layer& layer, const QSet<const NetSignal*>& netsignals,
                 bool ignorePlanes = false);
  void addVia(const BI_Via& via, const Length& offset = Length(0));
  void addNetLine(const BI_NetLine& netLine, const Length& offset = Length(0));
  void addPlane(const BI_Plane& plane);
  void addPolygon(const Path& path, const UnsignedLength& lineWidth,
                  bool filled);
  void addCircle(const Circle& circle, const Transform& transform,
                 const Length& offset = Length(0));
  void addStrokeText(const BI_StrokeText& strokeText,
                     const Length& offset = Length(0));
  void addHole(const PositiveLength& diameter, const NonEmptyPath& path,
               const Transform& transform = Transform(),
               const Length& offset = Length(0));
  void addPad(const BI_FootprintPad& pad, const Layer& layer,
              const Length& offset = Length(0));

private:  // Data
  Board& mBoard;
  PositiveLength mMaxArcTolerance;
  ClipperLib::Paths mPaths;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
