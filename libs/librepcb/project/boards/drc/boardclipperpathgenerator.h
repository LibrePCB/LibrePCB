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

#ifndef LIBREPCB_PROJECT_BOARDCLIPPERPATHGENERATOR_H
#define LIBREPCB_PROJECT_BOARDCLIPPERPATHGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/units/length.h>
#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class Board;
class NetSignal;

/*******************************************************************************
 *  Class BoardClipperPathGenerator
 ******************************************************************************/

/**
 * @brief The BoardClipperPathGenerator class creates a Clipper path from
 *        a ::librepcb::project::Board
 */
class BoardClipperPathGenerator final {
public:
  // Constructors / Destructor
  explicit BoardClipperPathGenerator(
      Board& board, const PositiveLength& maxArcTolerance) noexcept;
  ~BoardClipperPathGenerator() noexcept;

  // Getters
  const ClipperLib::Paths& getPaths() const noexcept { return mPaths; }

  // General Methods
  void addBoardOutline();
  void addHoles(const Length& offset);
  void addCopper(const QString& layerName, const NetSignal* netsignal);

private:  // Data
  Board&            mBoard;
  PositiveLength    mMaxArcTolerance;
  ClipperLib::Paths mPaths;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARDCLIPPERPATHGENERATOR_H
