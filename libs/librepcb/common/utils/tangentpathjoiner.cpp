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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "tangentpathjoiner.h"

#include "../geometry/path.h"

#include <functional>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVector<Path> TangentPathJoiner::join(QVector<Path> paths,
                                      qint64 timeoutMs) noexcept {
  QVector<Path> result;

  // Return closed paths as-is and skip invalid paths.
  for (int i = paths.count() - 1; i >= 0; --i) {
    if (paths.at(i).isClosed()) {
      result.append(paths.takeAt(i));
    } else if (paths.at(i).getVertices().count() < 2) {
      paths.removeAt(i);
    }
  }

  // Find all unambiguous path pairs which can be joined.
  enum class VertexIndex { First, Last };
  QHash<Point, QMap<int, VertexIndex> > joinPoints;
  for (int i = 0; i < paths.count(); ++i) {
    joinPoints[paths.at(i).getVertices().first().getPos()].insert(
        i, VertexIndex::First);
    joinPoints[paths.at(i).getVertices().last().getPos()].insert(
        i, VertexIndex::Last);
  }

  // Now join these pairs.
  QVector<int> obsoletePaths;
  for (auto i = joinPoints.begin(); i != joinPoints.end(); i++) {
    if (i.value().count() == 2) {
      int i1 = i.value().firstKey();
      int i2 = i.value().lastKey();
      VertexIndex p1 = i.value().first();
      VertexIndex p2 = i.value().last();
      QVector<Vertex> vertices;
      if ((p1 == VertexIndex::Last) && (p2 == VertexIndex::First)) {
        Q_ASSERT(paths.at(i1).getVertices().last().getPos() ==
                 paths.at(i2).getVertices().first().getPos());
        vertices = paths.at(i1).getVertices();
        vertices.takeLast();
        vertices.append(paths.at(i2).getVertices());
        joinPoints[vertices.last().getPos()].remove(i2);
        joinPoints[vertices.last().getPos()].insert(i1, VertexIndex::Last);
      } else if ((p1 == VertexIndex::Last) && (p2 == VertexIndex::Last)) {
        Q_ASSERT(paths.at(i1).getVertices().last().getPos() ==
                 paths.at(i2).getVertices().last().getPos());
        vertices = paths.at(i1).getVertices();
        vertices.takeLast();
        vertices.append(paths.at(i2).reversed().getVertices());
        joinPoints[vertices.last().getPos()].remove(i2);
        joinPoints[vertices.last().getPos()].insert(i1, VertexIndex::Last);
      } else if ((p1 == VertexIndex::First) && (p2 == VertexIndex::Last)) {
        Q_ASSERT(paths.at(i1).getVertices().first().getPos() ==
                 paths.at(i2).getVertices().last().getPos());
        vertices = paths.at(i2).getVertices();
        vertices.takeLast();
        vertices.append(paths.at(i1).getVertices());
        joinPoints[vertices.first().getPos()].remove(i2);
        joinPoints[vertices.first().getPos()].insert(i1, VertexIndex::First);
        joinPoints[vertices.last().getPos()].insert(i1, VertexIndex::Last);
      } else {
        Q_ASSERT(paths.at(i1).getVertices().first().getPos() ==
                 paths.at(i2).getVertices().first().getPos());
        vertices = paths.at(i1).reversed().getVertices();
        vertices.takeLast();
        vertices.append(paths.at(i2).getVertices());
        joinPoints[vertices.last().getPos()].remove(i2);
        joinPoints[vertices.last().getPos()].insert(i1, VertexIndex::Last);
        joinPoints[vertices.first().getPos()].insert(i1, VertexIndex::First);
      }
      paths[i1] = Path(vertices);
      Q_ASSERT(!obsoletePaths.contains(i2));
      obsoletePaths.append(i2);
    }
  }
  std::sort(obsoletePaths.begin(), obsoletePaths.end(), std::greater<int>());
  foreach (int i, obsoletePaths) { paths.removeAt(i); }

  // Add closed paths to the result and remove them from the input.
  for (int i = paths.count() - 1; i >= 0; --i) {
    if (paths.at(i).isClosed()) {
      result.append(paths.takeAt(i));
    }
  }

  // Find all paths and sort by relevance.
  QVector<Result> found;
  QElapsedTimer timer;
  timer.start();
  findAllPaths(found, paths, timer, timeoutMs);
  std::sort(found.begin(), found.end(), [](const Result& r1, const Result& r2) {
    // Prio 1: Closed paths
    if (r1.isClosed() != r2.isClosed()) {
      return r1.isClosed();
    }
    // Prio 2: Long paths
    if (r1.length != r2.length) {
      return r1.length > r2.length;
    }
    // Prio 3: Paths consisting of many joints
    int c1 = r1.segments.count();
    int c2 = r2.segments.count();
    if (c1 != c2) {
      return c1 > c2;
    }
    // Prio 4: Lower, non-reversed indices
    for (int i = 0; i < c1; ++i) {
      if (r1.segments.at(i).reverse != r2.segments.at(i).reverse) {
        return r2.segments.at(i).reverse;
      }
      if (r1.segments.at(i).index != r2.segments.at(i).index) {
        return r1.segments.at(i).index < r2.segments.at(i).index;
      }
    }
    return false;
  });

  // Add found paths to result.
  QSet<int> consumedIndices;
  foreach (const auto& f, found) {
    if ((f.indices & consumedIndices).isEmpty()) {
      result.append(f.buildPath(paths));
      consumedIndices.unite(f.indices);
    }
  }

  return result;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void TangentPathJoiner::findAllPaths(QVector<Result>& result,
                                     const QVector<Path>& paths,
                                     const QElapsedTimer& timer,
                                     qint64 timeoutMs,
                                     const Result& prefix) noexcept {
  for (int i = 0; i < paths.count(); ++i) {
    if ((timeoutMs >= 0) && (timer.elapsed() > timeoutMs)) {
      qWarning() << "TangentPathJoiner aborted due to timeout.";
      break;
    }
    if (!prefix.indices.contains(i)) {
      if (tl::optional<Result> r = join(paths, prefix, i, false)) {
        result.append(*r);
        if (!r->isClosed()) {
          findAllPaths(result, paths, timer, timeoutMs, *r);
        }
      }
      if (tl::optional<Result> r = join(paths, prefix, i, true)) {
        result.append(*r);
        if (!r->isClosed()) {
          findAllPaths(result, paths, timer, timeoutMs, *r);
        }
      }
    }
  }
}

tl::optional<TangentPathJoiner::Result> TangentPathJoiner::join(
    const QVector<Path>& paths, const Result& prefix, int index,
    bool reverse) noexcept {
  const Path& path = paths.at(index);
  const Point& start = reverse ? path.getVertices().last().getPos()
                               : path.getVertices().first().getPos();
  const Point& end = reverse ? path.getVertices().first().getPos()
                             : path.getVertices().last().getPos();
  if (prefix.segments.isEmpty()) {
    return prefix.sub(index, reverse, start, end,
                      path.getTotalStraightLength());
  } else if (start == prefix.endPos) {
    return prefix.sub(index, reverse, start, end,
                      path.getTotalStraightLength());
  } else {
    return tl::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
