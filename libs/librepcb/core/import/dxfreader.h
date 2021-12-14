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

#ifndef LIBREPCB_CORE_DXFREADER_H
#define LIBREPCB_CORE_DXFREADER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class DxfReaderImpl;
class FilePath;

/*******************************************************************************
 *  Class DxfReader
 ******************************************************************************/

/**
 * @brief Read DXF files into LibrePCB data structures
 *
 * Basically this is a wrapper around the third-party library "dxflib" to read
 * DXF files and return the DXF objects as LibrePCB data structures. See
 * documentation of dxflib about the exact capabilities of the reader itself.
 *
 * Our wrapper currently supports the following data structures:
 *   - Points
 *   - Circles
 *   - Lines (converted to polygons)
 *   - Arcs (converted to polygons)
 *   - Polylines (containing straight segments and arc segments)
 *
 * Note that this class tries to read and apply the length unit defined in the
 * DXF file. However, a DXF file is not required to specify the unit. If it is
 * missing, the unit millimeters is assumed.
 */
class DxfReader {
  Q_DECLARE_TR_FUNCTIONS(DxfReader)

public:
  struct Circle {
    Point position;
    PositiveLength diameter;
  };

  // Constructors / Destructor

  /**
   * @brief Default constructor
   */
  DxfReader() noexcept;

  DxfReader(const DxfReader& other) = delete;

  /**
   * @brief Destructor
   */
  virtual ~DxfReader() noexcept;

  // Setters

  /**
   * @brief Set a custom scale factor
   *
   * All imported objects will be scaled with this factor (default is 1.0).
   *
   * @param scaleFactor   Scale factor.
   */
  void setScaleFactor(qreal scaleFactor) noexcept {
    mScaleFactor = scaleFactor;
  }

  // Getters

  /**
   * @brief Get all imported points
   *
   * @return List of point objects
   */
  const QList<Point>& getPoints() const noexcept { return mPoints; }

  /**
   * @brief Get all imported circles
   *
   * @return List of circle objects
   */
  const QList<Circle>& getCircles() const noexcept { return mCircles; }

  /**
   * @brief Get all imported lines, arcs and polylines (converted to polygons)
   *
   * @return List of polygons
   */
  const QList<Path>& getPolygons() const noexcept { return mPolygons; }

  // General Methods

  /**
   * @brief Parse a DXF file
   *
   * @param dxfFile   File path to the DXF to import.
   *
   * @throw Exception if anything went wrong (e.g. file does not exist).
   */
  void parse(const FilePath& dxfFile);

  // Operator Overloadings
  DxfReader& operator=(const DxfReader& rhs) = delete;

private:
  qreal mScaleFactor;

  QList<Point> mPoints;
  QList<Circle> mCircles;
  QList<Path> mPolygons;

  /**
   * The actual implementation is in the *.cpp file and should have access to
   * this class members.
   */
  friend class DxfReaderImpl;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
