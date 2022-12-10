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

#ifndef LIBREPCB_CORE_PATH_H
#define LIBREPCB_CORE_PATH_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "vertex.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>
#include <QtWidgets>

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
class Path final {
  Q_DECLARE_TR_FUNCTIONS(Path)

public:
  // Constructors / Destructor
  Path() noexcept : mVertices(), mPainterPathPx() {}
  Path(const Path& other) noexcept;
  explicit Path(const QVector<Vertex>& vertices) noexcept
    : mVertices(vertices) {}
  explicit Path(const SExpression& node);
  ~Path() noexcept {}

  // Getters
  bool isClosed() const noexcept;
  bool isCurved() const noexcept;
  QVector<Vertex>& getVertices() noexcept {
    invalidatePainterPath();
    return mVertices;
  }
  const QVector<Vertex>& getVertices() const noexcept { return mVertices; }
  UnsignedLength getTotalStraightLength() const noexcept;
  Point calcNearestPointBetweenVertices(const Point& p) const noexcept;
  Path toClosedPath() const noexcept;
  QVector<Path> toOutlineStrokes(const PositiveLength& width) const noexcept;
  const QPainterPath& toQPainterPathPx() const noexcept;

  // Transformations
  Path& translate(const Point& offset) noexcept;
  Path translated(const Point& offset) const noexcept;
  Path& mapToGrid(const PositiveLength& gridInterval) noexcept;
  Path mappedToGrid(const PositiveLength& gridInterval) const noexcept;
  Path& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;
  Path rotated(const Angle& angle, const Point& center = Point(0, 0)) const
      noexcept;
  Path& mirror(Qt::Orientation orientation,
               const Point& center = Point(0, 0)) noexcept;
  Path mirrored(Qt::Orientation orientation,
                const Point& center = Point(0, 0)) const noexcept;
  Path& reverse() noexcept;
  Path reversed() const noexcept;

  // General Methods
  void addVertex(const Vertex& vertex) noexcept;
  void addVertex(const Point& pos, const Angle& angle = Angle::deg0()) noexcept;
  void insertVertex(int index, const Vertex& vertex) noexcept;
  void insertVertex(int index, const Point& pos,
                    const Angle& angle = Angle::deg0()) noexcept;
  bool close() noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  Path& operator=(const Path& rhs) noexcept;
  bool operator==(const Path& rhs) const noexcept {
    return mVertices == rhs.mVertices;
  }
  bool operator!=(const Path& rhs) const noexcept { return !(*this == rhs); }

  /**
   * @brief The "<" operator to compare two ::librepcb::Path objects
   *
   * Useful for sorting path lists/sets (e.g. to for canonical order in
   * files), or to store them in a QMap.
   *
   * @param rhs The right hand side object.
   *
   * @return true if this path is smaller, else false
   */
  bool operator<(const Path& rhs) const noexcept;

  // Static Methods
  static Path line(const Point& p1, const Point& p2,
                   const Angle& angle = Angle::deg0()) noexcept;
  static Path circle(const PositiveLength& diameter) noexcept;
  static Path obround(const PositiveLength& width,
                      const PositiveLength& height) noexcept;
  static Path obround(const Point& p1, const Point& p2,
                      const PositiveLength& width) noexcept;
  static Path arcObround(const Point& p1, const Point& p2, const Angle& angle,
                         const PositiveLength& width) noexcept;
  static Path rect(const Point& p1, const Point& p2) noexcept;
  static Path centeredRect(const PositiveLength& width,
                           const PositiveLength& height) noexcept;
  static Path octagon(const PositiveLength& width,
                      const PositiveLength& height) noexcept;
  static Path flatArc(const Point& p1, const Point& p2, const Angle& angle,
                      const PositiveLength& maxTolerance) noexcept;

  /**
   * @brief Convert multiple ::librepcb::Path objects to a QPainterPath
   *
   * The paths are united, so you get the union of all the passed paths.
   *
   * @param paths   The paths to convert.
   * @param area    Whether the passed paths should be interpreted as areas
   *                (true) or strokes (false).
   * @return        The QPainterPath with the united paths.
   */
  static QPainterPath toQPainterPathPx(const QVector<Path>& paths,
                                       bool area) noexcept;

private:  // Methods
  void invalidatePainterPath() const noexcept {
    mPainterPathPx = QPainterPath();
  }

private:  // Data
  QVector<Vertex> mVertices;
  mutable QPainterPath mPainterPathPx;  // cached path for #toQPainterPathPx()
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline uint qHash(const Path& key, uint seed = 0) noexcept {
  return qHashRange(key.getVertices().begin(), key.getVertices().end(), seed);
}

/*******************************************************************************
 *  Class NonEmptyPath
 ******************************************************************************/

struct NonEmptyPathVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val)
        ? std::forward<Value>(val)
        : (throw RuntimeError(__FILE__, __LINE__,
                              Path::tr("Path doesn't contain vertices!")),
           std::forward<Value>(val));
  }
};

struct NonEmptyPathConstraint {
  bool operator()(const Path& p) const noexcept {
    return p.getVertices().count() > 0;
  }
};

/**
 * NonEmptyPath is a wrapper around a librepcb::Path object which is
 * guaranteed to always contain at least one vertex.
 *
 * The constructor throws an exception if constructed from a librepcb::Path
 * without vertices.
 */
using NonEmptyPath = type_safe::constrained_type<Path, NonEmptyPathConstraint,
                                                 NonEmptyPathVerifier>;

inline uint qHash(const NonEmptyPath& key, uint seed = 0) noexcept {
  return ::qHash(*key, seed);
}

inline NonEmptyPath makeNonEmptyPath(const Point& pos) noexcept {
  return NonEmptyPath(Path({Vertex(pos)}));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
