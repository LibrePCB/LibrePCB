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

#ifndef LIBREPCB_CORE_TANGENTPATHJOINER_H
#define LIBREPCB_CORE_TANGENTPATHJOINER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TangentPathJoiner
 ******************************************************************************/

/**
 * @brief Helper class to join tangent paths (polylines) together
 *
 * The algorithm performs the following tasks:
 *
 *   - Invalid paths (less than 2 vertices) are removed.
 *   - Any already closed path is returned as-is.
 *   - Any joined, closed paths are searched, starting with the longest path.
 *   - Then joined, open paths are searched, starting with the longest path.
 *   - Any remaining (non tangent) paths are returned as-is.
 *
 * @note If there are many possible solutions (many paths located at the same
 *       coordinate), finding the solution can take a lot of time. Therefore
 *       a timeout can be specified to abort a too long operation, then a non-
 *       optimal (but still valid) result is returned.
 */
class TangentPathJoiner {
  Q_DECLARE_TR_FUNCTIONS(TangentPathJoiner)

public:
  // Constructors / Destructor
  TangentPathJoiner() = delete;
  TangentPathJoiner(const TangentPathJoiner& other) = delete;
  ~TangentPathJoiner() = delete;

  // General Methods
  static QVector<Path> join(QVector<Path> paths, qint64 timeoutMs = -1,
                            bool* timedOut = nullptr) noexcept;

  // Operator Overloadings
  TangentPathJoiner& operator=(const TangentPathJoiner& rhs) = delete;

private:
  struct Segment {
    int index;
    bool reverse;
  };

  struct Result {
    QVector<Segment> segments;
    QSet<int> indices;
    QSet<Point> junctions;
    Point startPos;
    Point endPos;
    mutable qreal lengthAreaCache;

    Result()
      : segments(),
        indices(),
        junctions(),
        startPos(),
        endPos(),
        lengthAreaCache() {}

    bool isClosed() const noexcept {
      return (!segments.isEmpty()) && (startPos == endPos);
    }

    qreal calcLengthOrArea(const QVector<Path>& paths) const noexcept {
      if (lengthAreaCache == 0) {
        const Path path = buildPath(paths);
        lengthAreaCache = path.isClosed()
            ? path.calcAreaOfStraightSegments()
            : path.getTotalStraightLength()->toMm();
      }
      return lengthAreaCache;
    }

    Result sub(int index, bool reverse, const Point& start,
               const Point& end) const {
      Result r(*this);
      r.segments.append(Segment{index, reverse});
      r.indices.insert(index);
      r.junctions.insert(end);
      if (segments.isEmpty()) {
        r.startPos = start;
      }
      r.endPos = end;
      return r;
    }

    Path buildPath(const QVector<Path>& paths) const {
      QVector<Vertex> vertices;
      foreach (const Segment& segment, segments) {
        if (!vertices.isEmpty()) {
          vertices.takeLast();
        }
        Path p = paths.at(segment.index);
        if (segment.reverse) {
          p.reverse();
        }
        vertices.append(p.getVertices());
      }
      return Path(vertices);
    }
  };

  static void findAllPaths(QVector<Result>& result, const QVector<Path>& paths,
                           const QElapsedTimer& timer, qint64 timeoutMs,
                           const Result& prefix = Result(),
                           bool* timedOut = nullptr) noexcept;

  static tl::optional<Result> join(const QVector<Path>& paths,
                                   const Result& prefix, int index,
                                   bool reverse) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
