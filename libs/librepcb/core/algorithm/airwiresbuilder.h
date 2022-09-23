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

#ifndef LIBREPCB_CORE_AIRWIRESBUILDER_H
#define LIBREPCB_CORE_AIRWIRESBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AirWiresBuilderImpl;

/*******************************************************************************
 *  Class AirWiresBuilder
 ******************************************************************************/

/**
 * @brief The AirWiresBuilder class
 */
class AirWiresBuilder final {
  Q_DECLARE_TR_FUNCTIONS(AirWiresBuilder)

public:
  // Types
  typedef QPair<Point, Point> AirWire;
  typedef QVector<AirWire> AirWires;

  // Constructors / Destructor

  /**
   * @brief Default constructor
   */
  AirWiresBuilder() noexcept;

  /**
   * @brief Copy constructor
   *
   * @param other     Another ::librepcb::AirWiresBuilder object
   */
  AirWiresBuilder(const AirWiresBuilder& other) = delete;

  /**
   * Destructor
   */
  ~AirWiresBuilder() noexcept;

  /**
   * @brief Add a new point
   *
   * @param p   The point to add
   *
   * @return The ID of the added point
   */
  int addPoint(const Point& p) noexcept;

  /**
   * @brief Add an edge between two points
   *
   * @param p1  ID of first point
   * @param p2  ID of second point
   */
  void addEdge(int p1, int p2) noexcept;

  /**
   * @brief Build the air wires
   *
   * @return Coordinates of air wires
   */
  AirWires buildAirWires() noexcept;

  // Operator overloadings
  AirWiresBuilder& operator=(const AirWiresBuilder& rhs) = delete;

private:  // Data
  /**
   * The actual implementation is in the *.cpp file to make the dependency to
   * the delaunay triangulation library a private implementation detail. So
   * all other libraries do not need this dependency.
   */
  QScopedPointer<AirWiresBuilderImpl> mImpl;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
