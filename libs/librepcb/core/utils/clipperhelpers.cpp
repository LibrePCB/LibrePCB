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
#include "clipperhelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ClipperHelpers::unite(ClipperLib::Paths& paths) {
  try {
    ClipperLib::Clipper c;
    c.AddPaths(paths, ClipperLib::ptSubject, true);
    c.Execute(ClipperLib::ctUnion, paths, ClipperLib::pftEvenOdd,
              ClipperLib::pftEvenOdd);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::unite(ClipperLib::Paths& subject,
                           const ClipperLib::Path& clip) {
  try {
    ClipperLib::Clipper c;
    c.AddPaths(subject, ClipperLib::ptSubject, true);
    c.AddPath(clip, ClipperLib::ptClip, true);
    c.Execute(ClipperLib::ctUnion, subject, ClipperLib::pftEvenOdd,
              ClipperLib::pftEvenOdd);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::unite(ClipperLib::Paths& subject,
                           const ClipperLib::Paths& clip) {
  try {
    ClipperLib::Clipper c;
    c.AddPaths(subject, ClipperLib::ptSubject, true);
    c.AddPaths(clip, ClipperLib::ptClip, true);
    c.Execute(ClipperLib::ctUnion, subject, ClipperLib::pftEvenOdd,
              ClipperLib::pftEvenOdd);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

std::unique_ptr<ClipperLib::PolyTree> ClipperHelpers::intersect(
    const ClipperLib::Paths& subject, const ClipperLib::Paths& clip) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<ClipperLib::PolyTree> result(new ClipperLib::PolyTree());
    ClipperLib::Clipper c;
    c.AddPaths(subject, ClipperLib::ptSubject, true);
    c.AddPaths(clip, ClipperLib::ptClip, true);
    c.Execute(ClipperLib::ctIntersection, *result, ClipperLib::pftEvenOdd,
              ClipperLib::pftEvenOdd);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to intersect paths: %1").arg(e.what()));
  }
}

std::unique_ptr<ClipperLib::PolyTree> ClipperHelpers::intersect(
    const QList<ClipperLib::Paths>& paths) {
  try {
    // Intersection makes no sense with less than two areas (and thus method
    // wouldn't work in that case).
    if (paths.count() < 2) {
      throw LogicError(__FILE__, __LINE__, "Less than two areas specified.");
    }

    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<ClipperLib::PolyTree> result(new ClipperLib::PolyTree());
    ClipperLib::Clipper c;
    ClipperLib::Paths intermediateSubject;
    for (int i = 1; i < paths.count(); ++i) {
      c.Clear();
      if (i == 1) {
        c.AddPaths(paths.first(), ClipperLib::ptSubject, true);
      } else {
        ClipperLib::PolyTreeToPaths(*result, intermediateSubject);
        c.AddPaths(intermediateSubject, ClipperLib::ptSubject, true);
      }
      c.AddPaths(paths.at(i), ClipperLib::ptClip, true);
      c.Execute(ClipperLib::ctIntersection, *result, ClipperLib::pftEvenOdd,
                ClipperLib::pftEvenOdd);
    }
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to intersect paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::subtract(ClipperLib::Paths& subject,
                              const ClipperLib::Paths& clip) {
  try {
    ClipperLib::Clipper c;
    c.AddPaths(subject, ClipperLib::ptSubject, true);
    c.AddPaths(clip, ClipperLib::ptClip, true);
    c.Execute(ClipperLib::ctDifference, subject, ClipperLib::pftEvenOdd,
              ClipperLib::pftEvenOdd);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to subtract paths: %1").arg(e.what()));
  }
}

std::unique_ptr<ClipperLib::PolyTree> ClipperHelpers::subtractToTree(
    const ClipperLib::Paths& subject, const ClipperLib::Paths& clip) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<ClipperLib::PolyTree> result(new ClipperLib::PolyTree());
    ClipperLib::Clipper c;
    c.AddPaths(subject, ClipperLib::ptSubject, true);
    c.AddPaths(clip, ClipperLib::ptClip, true);
    c.Execute(ClipperLib::ctDifference, *result, ClipperLib::pftEvenOdd,
              ClipperLib::pftEvenOdd);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to subtract paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::offset(ClipperLib::Paths& paths, const Length& offset,
                            const PositiveLength& maxArcTolerance) {
  try {
    ClipperLib::ClipperOffset o(2.0, maxArcTolerance->toNm());
    o.AddPaths(paths, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    o.Execute(paths, offset.toNm());
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to offset a path: %1").arg(e.what()));
  }
}

ClipperLib::Paths ClipperHelpers::treeToPaths(
    const ClipperLib::PolyTree& tree) {
  try {
    ClipperLib::Paths paths;
    ClipperLib::PolyTreeToPaths(tree, paths);
    return paths;
  } catch (const std::exception& e) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("Failed to convert clipper tree to paths: %1").arg(e.what()));
  }
}

ClipperLib::Paths ClipperHelpers::flattenTree(
    const ClipperLib::PolyNode& node) {
  ClipperLib::Paths paths;
  for (const ClipperLib::PolyNode* outlineChild : node.Childs) {
    Q_ASSERT(outlineChild);
    if (outlineChild->IsHole()) throw LogicError(__FILE__, __LINE__);
    ClipperLib::Paths holes;
    for (ClipperLib::PolyNode* holeChild : outlineChild->Childs) {
      Q_ASSERT(holeChild);
      if (!holeChild->IsHole()) throw LogicError(__FILE__, __LINE__);
      holes.push_back(holeChild->Contour);
      ClipperLib::Paths subpaths = flattenTree(*holeChild);  // can throw
      paths.insert(paths.end(), subpaths.begin(), subpaths.end());
    }
    paths.push_back(
        convertHolesToCutIns(outlineChild->Contour, holes));  // can throw
  }
  return paths;
}

/*******************************************************************************
 *  Conversion Methods
 ******************************************************************************/

QVector<Path> ClipperHelpers::convert(const ClipperLib::Paths& paths) noexcept {
  QVector<Path> p;
  p.reserve(paths.size());
  for (const ClipperLib::Path& path : paths) {
    p.append(convert(path));
  }
  return p;
}

Path ClipperHelpers::convert(const ClipperLib::Path& path) noexcept {
  Path p;
  for (const ClipperLib::IntPoint& point : path) {
    p.addVertex(convert(point));
  }
  p.close();
  return p;
}

Point ClipperHelpers::convert(const ClipperLib::IntPoint& point) noexcept {
  return Point(point.X, point.Y);
}

ClipperLib::Paths ClipperHelpers::convert(
    const QVector<Path>& paths,
    const PositiveLength& maxArcTolerance) noexcept {
  ClipperLib::Paths p;
  p.reserve(paths.size());
  foreach (const Path& path, paths) {
    p.push_back(convert(path, maxArcTolerance));
  }
  return p;
}

ClipperLib::Path ClipperHelpers::convert(
    const Path& path, const PositiveLength& maxArcTolerance) noexcept {
  ClipperLib::Path p;
  for (int i = 0; i < path.getVertices().count(); ++i) {
    const Vertex& v = path.getVertices().at(i);
    const Vertex& v0 = path.getVertices().at(qMax(i - 1, 0));
    if ((i == 0) || (v0.getAngle() == 0)) {
      p.push_back(convert(v.getPos()));
    } else {
      // approximate arcs by many short straight line segments
      Path arc = Path::flatArc(v0.getPos(), v.getPos(), v0.getAngle(),
                               maxArcTolerance);
      // skip first point as it is would be a duplicate
      for (int k = 1; k < arc.getVertices().count(); ++k) {
        p.push_back(convert(arc.getVertices().at(k).getPos()));
      }
    }
  }
  // make sure all paths have the same orientation, otherwise we get strange
  // results
  if (!ClipperLib::Orientation(p)) {
    ClipperLib::ReversePath(p);
  }
  return p;
}

ClipperLib::IntPoint ClipperHelpers::convert(const Point& point) noexcept {
  return ClipperLib::IntPoint(point.getX().toNm(), point.getY().toNm());
}

/*******************************************************************************
 *  Internal Helper Methods
 ******************************************************************************/

ClipperLib::Path ClipperHelpers::convertHolesToCutIns(
    const ClipperLib::Path& outline, const ClipperLib::Paths& holes) {
  ClipperLib::Path path = outline;
  ClipperLib::Paths preparedHoles = prepareHoles(holes);
  for (const ClipperLib::Path& hole : preparedHoles) {
    addCutInToPath(path, hole);  // can throw
  }
  return path;
}

ClipperLib::Paths ClipperHelpers::prepareHoles(
    const ClipperLib::Paths& holes) noexcept {
  ClipperLib::Paths preparedHoles;
  for (const ClipperLib::Path& hole : holes) {
    if (hole.size() > 2) {
      preparedHoles.push_back(rotateCutInHole(hole));
    } else {
      qWarning() << "Detected invalid hole in path flattening algorithm, "
                    "ignoring it.";
    }
  }
  // important: sort holes by the y coordinate of their connection point
  // (to make sure no cut-ins are overlapping in the resulting plane)
  std::sort(preparedHoles.begin(), preparedHoles.end(),
            [](const ClipperLib::Path& p1, const ClipperLib::Path& p2) {
              return p1.front().Y < p2.front().Y;
            });
  return preparedHoles;
}

ClipperLib::Path ClipperHelpers::rotateCutInHole(
    const ClipperLib::Path& hole) noexcept {
  ClipperLib::Path p = hole;
  if (p.back() == p.front()) {
    p.pop_back();
  }
  std::rotate(p.begin(), p.begin() + getHoleConnectionPointIndex(p), p.end());
  return p;
}

int ClipperHelpers::getHoleConnectionPointIndex(
    const ClipperLib::Path& hole) noexcept {
  int index = 0;
  for (size_t i = 1; i < hole.size(); ++i) {
    if (hole.at(i).Y < hole.at(index).Y) {
      index = i;
    }
  }
  return index;
}

void ClipperHelpers::addCutInToPath(ClipperLib::Path& outline,
                                    const ClipperLib::Path& hole) {
  int index = insertConnectionPointToPath(outline, hole.front());  // can throw
  outline.insert(outline.begin() + index, hole.begin(), hole.end());
}

int ClipperHelpers::insertConnectionPointToPath(ClipperLib::Path& path,
                                                const ClipperLib::IntPoint& p) {
  int nearestIndex = -1;
  ClipperLib::IntPoint nearestPoint;
  for (size_t i = 0; i < path.size(); ++i) {
    ClipperLib::cInt y;
    if (calcIntersectionPos(path.at(i), path.at((i + 1) % path.size()), p.X,
                            y)) {
      if ((y <= p.Y) &&
          ((nearestIndex < 0) || (p.Y - y < p.Y - nearestPoint.Y))) {
        nearestIndex = i;
        nearestPoint = ClipperLib::IntPoint(p.X, y);
      }
    }
  }
  if (nearestIndex >= 0) {
    path.insert(path.begin() + nearestIndex + 1, nearestPoint);
    path.insert(path.begin() + nearestIndex + 1, p);
    path.insert(path.begin() + nearestIndex + 1, nearestPoint);
    return nearestIndex + 2;
  } else {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to calculate the connection point of a "
                             "cut-in to an outline!"));
  }
}

bool ClipperHelpers::calcIntersectionPos(const ClipperLib::IntPoint& p1,
                                         const ClipperLib::IntPoint& p2,
                                         const ClipperLib::cInt& x,
                                         ClipperLib::cInt& y) noexcept {
  if (((p1.X <= x) && (p2.X > x)) || ((p1.X >= x) && (p2.X < x))) {
    qreal yCalc =
        p1.Y + (qreal(x - p1.X) * qreal(p2.Y - p1.Y) / qreal(p2.X - p1.X));
    y = qBound(qMin(p1.Y, p2.Y), ClipperLib::cInt(yCalc), qMax(p1.Y, p2.Y));
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
