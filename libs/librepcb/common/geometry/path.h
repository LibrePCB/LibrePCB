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

#ifndef LIBREPCB_PATH_H
#define LIBREPCB_PATH_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "vertex.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Path
 ******************************************************************************/

/**
 * @brief The Path class represents a list of vertices connected by straight
 * lines or circular arc segments
 *
 * This class is similar to a polygon or polyline, but it doesn't have
 * properties like width or layer. It's only a list of coordinates which are
 * connected together by straight lines or circular arc segments.
 *
 * A path may be closed (first point == last point) or open (first point != last
 * point).
 *
 * For a valid path, minimum two vertices are required. Paths with less than two
 * vertices are useless and thus considered as invalid.
 */
class Path final : public SerializableObject {
public:
  // Constructors / Destructor
  Path() noexcept : mVertices(), mPainterPathPx() {}
  Path(const Path& other) noexcept;
  explicit Path(const QVector<Vertex>& vertices) noexcept
    : mVertices(vertices) {}
  explicit Path(const SExpression& node);
  ~Path() noexcept {}

  // Getters
  bool             isClosed() const noexcept;
  QVector<Vertex>& getVertices() noexcept {
    invalidatePainterPath();
    return mVertices;
  }
  const QVector<Vertex>& getVertices() const noexcept { return mVertices; }
  const QPainterPath&    toQPainterPathPx(bool close = false) const noexcept;

  // Transformations
  Path& translate(const Point& offset) noexcept;
  Path  translated(const Point& offset) const noexcept;
  Path& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;
  Path  rotated(const Angle& angle, const Point& center = Point(0, 0)) const
      noexcept;
  Path& mirror(Qt::Orientation orientation,
               const Point&    center = Point(0, 0)) noexcept;
  Path  mirrored(Qt::Orientation orientation,
                 const Point&    center = Point(0, 0)) const noexcept;

  // General Methods
  void addVertex(const Vertex& vertex) noexcept;
  void addVertex(const Point& pos, const Angle& angle = Angle::deg0()) noexcept;
  void insertVertex(int index, const Vertex& vertex) noexcept;
  void insertVertex(int index, const Point& pos,
                    const Angle& angle = Angle::deg0()) noexcept;
  bool close() noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Path& rhs) const noexcept {
    return mVertices == rhs.mVertices;
  }
  bool  operator!=(const Path& rhs) const noexcept { return !(*this == rhs); }
  Path& operator=(const Path& rhs) noexcept;

  // Static Methods
  static Path line(const Point& p1, const Point& p2,
                   const Angle& angle = Angle::deg0()) noexcept;
  static Path circle(const PositiveLength& diameter) noexcept;
  static Path obround(const PositiveLength& width,
                      const PositiveLength& height) noexcept;
  static Path obround(const Point& p1, const Point& p2,
                      const PositiveLength& width) noexcept;
  static Path rect(const Point& p1, const Point& p2) noexcept;
  static Path centeredRect(const PositiveLength& width,
                           const PositiveLength& height) noexcept;
  static Path octagon(const PositiveLength& width,
                      const PositiveLength& height) noexcept;
  static Path flatArc(const Point& p1, const Point& p2, const Angle& angle,
                      const PositiveLength& maxTolerance) noexcept;
  static QPainterPath toQPainterPathPx(const QVector<Path>& paths) noexcept;

private:  // Methods
  void invalidatePainterPath() const noexcept {
    mPainterPathPx = QPainterPath();
  }

private:  // Data
  QVector<Vertex>      mVertices;
  mutable QPainterPath mPainterPathPx;  // cached path for #toQPainterPathPx()
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline uint qHash(const Path& key, uint seed = 0) noexcept {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
  return qHashRange(key.getVertices().begin(), key.getVertices().end(), seed);
#else
  uint hash = seed;
  foreach (const Vertex& v, key.getVertices()) {
    // from
    // https://code.woboq.org/qt5/qtbase/src/corelib/tools/qhashfunctions.h.html
    hash += seed ^ (qHash(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  }
  return hash;
#endif
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_PATH_H
