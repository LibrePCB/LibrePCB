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

bool ClipperHelpers::allPointsInside(const Clipper2Lib::Path64& points,
                                     const Clipper2Lib::Path64& path) {
  try {
    for (const Clipper2Lib::Point64& p : points) {
      if (Clipper2Lib::PointInPolygon(p, path) ==
          Clipper2Lib::PointInPolygonResult::IsOutside) {
        return false;
      }
    }
    return true;
  } catch (const std::exception& e) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("ClipperHelpers::allPointsInside() failed: %1").arg(e.what()));
  }
}

bool ClipperHelpers::anyPointsInside(const Clipper2Lib::Path64& points,
                                     const Clipper2Lib::Path64& path) {
  try {
    for (const Clipper2Lib::Point64& point : points) {
      if (Clipper2Lib::PointInPolygon(point, path) ==
          Clipper2Lib::PointInPolygonResult::IsInside) {
        return true;
      }
    }
    return false;
  } catch (const std::exception& e) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("ClipperHelpers::anyPointsInside() failed: %1").arg(e.what()));
  }
}

bool ClipperHelpers::anyPointsInside(const Clipper2Lib::Paths64& points,
                                     const Clipper2Lib::Path64& path) {
  try {
    for (const Clipper2Lib::Path64& pointsPath : points) {
      for (const Clipper2Lib::Point64& point : pointsPath) {
        if (Clipper2Lib::PointInPolygon(point, path) ==
            Clipper2Lib::PointInPolygonResult::IsInside) {
          return true;
        }
      }
    }
    return false;
  } catch (const std::exception& e) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("ClipperHelpers::anyPointsInside() failed: %1").arg(e.what()));
  }
}

void ClipperHelpers::unite(Clipper2Lib::Paths64& paths,
                           Clipper2Lib::FillRule fillType) {
  try {
    Clipper2Lib::Clipper64 c;
    c.AddSubject(paths);
    c.Execute(Clipper2Lib::ClipType::Union, fillType, paths);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::unite(Clipper2Lib::Paths64& subject,
                           const Clipper2Lib::Paths64& clip,
                           Clipper2Lib::FillRule subjectFillType,
                           Clipper2Lib::FillRule clipFillType) {
  try {
    Clipper2Lib::Clipper64 c;
    c.AddSubject(subject);
    c.AddClip(clip);
    c.Execute(Clipper2Lib::ClipType::Union, clipFillType, subject);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

std::unique_ptr<Clipper2Lib::PolyTree64> ClipperHelpers::uniteToTree(
    const Clipper2Lib::Paths64& paths, Clipper2Lib::FillRule fillType) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<Clipper2Lib::PolyTree64> result(
        new Clipper2Lib::PolyTree64());
    Clipper2Lib::Clipper64 c;
    c.AddSubject(paths);
    c.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::EvenOdd,
              *result);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

std::unique_ptr<Clipper2Lib::PolyTree64> ClipperHelpers::uniteToTree(
    const Clipper2Lib::Paths64& paths, const Clipper2Lib::Paths64& clip,
    Clipper2Lib::FillRule subjectFillType, Clipper2Lib::FillRule clipFillType) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<Clipper2Lib::PolyTree64> result(
        new Clipper2Lib::PolyTree64());
    Clipper2Lib::Clipper64 c;
    c.AddSubject(paths);
    c.AddClip(clip);
    c.Execute(Clipper2Lib::ClipType::Union, clipFillType, *result);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to unite paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::intersect(Clipper2Lib::Paths64& subject,
                               const Clipper2Lib::Paths64& clip,
                               Clipper2Lib::FillRule subjectFillType,
                               Clipper2Lib::FillRule clipFillType) {
  try {
    Clipper2Lib::Clipper64 c;
    c.AddSubject(subject);
    c.AddClip(clip);
    c.Execute(Clipper2Lib::ClipType::Intersection, clipFillType, subject);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to intersect paths: %1").arg(e.what()));
  }
}

std::unique_ptr<Clipper2Lib::PolyTree64> ClipperHelpers::intersectToTree(
    const Clipper2Lib::Paths64& subject, const Clipper2Lib::Paths64& clip,
    Clipper2Lib::FillRule subjectFillType, Clipper2Lib::FillRule clipFillType,
    bool closed) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<Clipper2Lib::PolyTree64> result(
        new Clipper2Lib::PolyTree64());
    Clipper2Lib::Clipper64 c;
    if (closed) {
      c.AddSubject(subject);
    } else {
      c.AddOpenSubject(subject);
    }
    c.AddClip(clip);
    c.Execute(Clipper2Lib::ClipType::Intersection, clipFillType, *result);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to intersect paths: %1").arg(e.what()));
  }
}

std::unique_ptr<Clipper2Lib::PolyTree64> ClipperHelpers::intersectToTree(
    const QList<Clipper2Lib::Paths64>& paths) {
  try {
    // Intersection makes no sense with less than two areas (and thus method
    // wouldn't work in that case).
    if (paths.count() < 2) {
      throw LogicError(__FILE__, __LINE__, "Less than two areas specified.");
    }

    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<Clipper2Lib::PolyTree64> result(
        new Clipper2Lib::PolyTree64());
    Clipper2Lib::Clipper64 c;
    Clipper2Lib::Paths64 intermediateSubject;
    for (int i = 1; i < paths.count(); ++i) {
      c.Clear();
      if (i == 1) {
        c.AddSubject(paths.first());
      } else {
        intermediateSubject = Clipper2Lib::PolyTreeToPaths64(*result);
        c.AddSubject(intermediateSubject);
      }
      c.AddClip(paths.at(i));
      c.Execute(Clipper2Lib::ClipType::Intersection,
                Clipper2Lib::FillRule::EvenOdd, *result);
    }
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to intersect paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::subtract(Clipper2Lib::Paths64& subject,
                              const Clipper2Lib::Paths64& clip,
                              Clipper2Lib::FillRule subjectFillType,
                              Clipper2Lib::FillRule clipFillType) {
  try {
    Clipper2Lib::Clipper64 c;
    c.AddSubject(subject);
    c.AddClip(clip);
    c.Execute(Clipper2Lib::ClipType::Difference, clipFillType, subject);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to subtract paths: %1").arg(e.what()));
  }
}

std::unique_ptr<Clipper2Lib::PolyTree64> ClipperHelpers::subtractToTree(
    const Clipper2Lib::Paths64& subject, const Clipper2Lib::Paths64& clip,
    Clipper2Lib::FillRule subjectFillType, Clipper2Lib::FillRule clipFillType,
    bool closed) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<Clipper2Lib::PolyTree64> result(
        new Clipper2Lib::PolyTree64());
    Clipper2Lib::Clipper64 c;
    if (closed) {
      c.AddSubject(subject);
    } else {
      c.AddOpenSubject(subject);
    }
    c.AddClip(clip);
    c.Execute(Clipper2Lib::ClipType::Difference, clipFillType, *result);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to subtract paths: %1").arg(e.what()));
  }
}

void ClipperHelpers::offset(Clipper2Lib::Paths64& paths, const Length& offset,
                            const PositiveLength& maxArcTolerance,
                            Clipper2Lib::JoinType joinType) {
  try {
    Clipper2Lib::ClipperOffset o(2.0, maxArcTolerance->toNm());
    o.AddPaths(paths, joinType, Clipper2Lib::EndType::Polygon);
    o.Execute(offset.toNm(), paths);
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to offset a path: %1").arg(e.what()));
  }
}

std::unique_ptr<Clipper2Lib::PolyTree64> ClipperHelpers::offsetToTree(
    const Clipper2Lib::Paths64& paths, const Length& offset,
    const PositiveLength& maxArcTolerance) {
  try {
    // Wrap the PolyTree object in a smart pointer since PolyTree cannot
    // safely be copied (i.e. returned by value), it would lead to a crash!!!
    std::unique_ptr<Clipper2Lib::PolyTree64> result(
        new Clipper2Lib::PolyTree64());
    Clipper2Lib::ClipperOffset o(2.0, maxArcTolerance->toNm());
    o.AddPaths(paths, Clipper2Lib::JoinType::Round,
               Clipper2Lib::EndType::Polygon);
    o.Execute(offset.toNm(), *result);
    return result;
  } catch (const std::exception& e) {
    throw LogicError(__FILE__, __LINE__,
                     QString("Failed to offset paths: %1").arg(e.what()));
  }
}

Clipper2Lib::Paths64 ClipperHelpers::treeToPaths(
    const Clipper2Lib::PolyTree64& tree) {
  try {
    return Clipper2Lib::PolyTreeToPaths64(tree);
  } catch (const std::exception& e) {
    throw LogicError(
        __FILE__, __LINE__,
        QString("Failed to convert clipper tree to paths: %1").arg(e.what()));
  }
}

Clipper2Lib::Paths64 ClipperHelpers::flattenTree(
    const Clipper2Lib::PolyTree64& node) {
  Clipper2Lib::Paths64 paths;
  for (const auto& outlineChild : node) {
    Q_ASSERT(outlineChild);
    if (outlineChild->IsHole()) throw LogicError(__FILE__, __LINE__);
    Clipper2Lib::Paths64 holes;
    for (const auto& holeChild : *outlineChild) {
      Q_ASSERT(holeChild);
      if (!holeChild->IsHole()) throw LogicError(__FILE__, __LINE__);
      holes.push_back(holeChild->Polygon());
      Clipper2Lib::Paths64 subpaths = flattenTree(*holeChild);  // can throw
      paths.insert(paths.end(), subpaths.begin(), subpaths.end());
    }
    paths.push_back(
        convertHolesToCutIns(outlineChild->Polygon(), holes));  // can throw
  }
  return paths;
}

/*******************************************************************************
 *  Conversion Methods
 ******************************************************************************/

QVector<Path> ClipperHelpers::convert(
    const Clipper2Lib::Paths64& paths) noexcept {
  QVector<Path> p;
  p.reserve(paths.size());
  for (const Clipper2Lib::Path64& path : paths) {
    p.append(convert(path));
  }
  return p;
}

Path ClipperHelpers::convert(const Clipper2Lib::Path64& path) noexcept {
  Path p;
  for (const Clipper2Lib::Point64& point : path) {
    p.addVertex(convert(point));
  }
  p.close();
  return p;
}

Point ClipperHelpers::convert(const Clipper2Lib::Point64& point) noexcept {
  return Point(point.x, point.y);
}

Clipper2Lib::Paths64 ClipperHelpers::convert(
    const QVector<Path>& paths,
    const PositiveLength& maxArcTolerance) noexcept {
  Clipper2Lib::Paths64 p;
  p.reserve(paths.size());
  foreach (const Path& path, paths) {
    p.push_back(convert(path, maxArcTolerance));
  }
  return p;
}

Clipper2Lib::Path64 ClipperHelpers::convert(
    const Path& path, const PositiveLength& maxArcTolerance) noexcept {
  Clipper2Lib::Path64 p;
  foreach (const Vertex& v, path.flattenedArcs(maxArcTolerance).getVertices()) {
    p.push_back(convert(v.getPos()));
  }
  // make sure all paths have the same orientation, otherwise we get strange
  // results
  if (!Clipper2Lib::IsPositive(p)) {
    std::reverse(p.begin(), p.end());
  }
  return p;
}

Clipper2Lib::Point64 ClipperHelpers::convert(const Point& point) noexcept {
  return Clipper2Lib::Point64(point.getX().toNm(), point.getY().toNm());
}

/*******************************************************************************
 *  Internal Helper Methods
 ******************************************************************************/

Clipper2Lib::Path64 ClipperHelpers::convertHolesToCutIns(
    const Clipper2Lib::Path64& outline, const Clipper2Lib::Paths64& holes) {
  Clipper2Lib::Path64 path = outline;
  Clipper2Lib::Paths64 preparedHoles = prepareHoles(holes);
  for (const Clipper2Lib::Path64& hole : preparedHoles) {
    addCutInToPath(path, hole);  // can throw
  }
  // Remove duplicates which might have been created by cut-ins.
  for (std::size_t i = (path.size() - 1); i > 0; --i) {
    if (path.at(i) == path.at(i - 1)) {
      path.erase(path.begin() + i);
    }
  }
  return path;
}

Clipper2Lib::Paths64 ClipperHelpers::prepareHoles(
    const Clipper2Lib::Paths64& holes) noexcept {
  Clipper2Lib::Paths64 preparedHoles;
  for (const Clipper2Lib::Path64& hole : holes) {
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
            [](const Clipper2Lib::Path64& p1, const Clipper2Lib::Path64& p2) {
              return p1.front().y < p2.front().y;
            });
  return preparedHoles;
}

Clipper2Lib::Path64 ClipperHelpers::rotateCutInHole(
    const Clipper2Lib::Path64& hole) noexcept {
  Clipper2Lib::Path64 p = hole;
  if (p.back() == p.front()) {
    p.pop_back();
  }
  auto minIt = std::min_element(
      p.begin(), p.end(),
      [](const Clipper2Lib::Point64& a, const Clipper2Lib::Point64& b) {
        return (a.y < b.y) || ((a.y == b.y) && (a.x < b.x));
      });
  std::rotate(p.begin(), minIt, p.end());
  return p;
}

void ClipperHelpers::addCutInToPath(Clipper2Lib::Path64& outline,
                                    const Clipper2Lib::Path64& hole) {
  int index = insertConnectionPointToPath(outline, hole.front());  // can throw
  outline.insert(outline.begin() + index, hole.begin(), hole.end());
}

int ClipperHelpers::insertConnectionPointToPath(Clipper2Lib::Path64& path,
                                                const Clipper2Lib::Point64& p) {
  int nearestIndex = -1;
  Clipper2Lib::Point64 nearestPoint;
  for (size_t i = 0; i < path.size(); ++i) {
    int64_t y;
    if (calcIntersectionPos(path.at(i), path.at((i + 1) % path.size()), p.x,
                            y)) {
      if ((y <= p.y) &&
          ((nearestIndex < 0) || (p.y - y < p.y - nearestPoint.y))) {
        nearestIndex = i;
        nearestPoint = Clipper2Lib::Point64(p.x, y);
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

bool ClipperHelpers::calcIntersectionPos(const Clipper2Lib::Point64& p1,
                                         const Clipper2Lib::Point64& p2,
                                         const int64_t& x,
                                         int64_t& y) noexcept {
  if (((p1.x <= x) && (p2.x > x)) || ((p1.x >= x) && (p2.x < x))) {
    qreal yCalc =
        p1.y + (qreal(x - p1.x) * qreal(p2.y - p1.y) / qreal(p2.x - p1.x));
    y = qBound(qMin(p1.y, p2.y), int64_t(yCalc), qMax(p1.y, p2.y));
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
