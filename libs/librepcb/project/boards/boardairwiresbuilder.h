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

#ifndef LIBREPCB_PROJECT_BOARDAIRWIRESBUILDER_H
#define LIBREPCB_PROJECT_BOARDAIRWIRESBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/units/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class NetSignal;
class Board;

/*******************************************************************************
 *  Class BoardAirWiresBuilder
 ******************************************************************************/

/**
 * @brief The BoardAirWiresBuilder class
 */
class BoardAirWiresBuilder final {
public:
  // Constructors / Destructor
  BoardAirWiresBuilder()                                  = delete;
  BoardAirWiresBuilder(const BoardAirWiresBuilder& other) = delete;
  BoardAirWiresBuilder(const Board& board, const NetSignal& netsignal) noexcept;
  ~BoardAirWiresBuilder() noexcept;

  // General Methods
  QVector<QPair<Point, Point>> buildAirWires() const;

  // Operator Overloadings
  BoardAirWiresBuilder& operator=(const BoardAirWiresBuilder& rhs) = delete;

private:  // Data
  const Board&     mBoard;
  const NetSignal& mNetSignal;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARDAIRWIRESBUILDER_H
