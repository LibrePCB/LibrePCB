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

#include "polygonsimplifier.h"

#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename LibElemType>
PolygonSimplifier<LibElemType>::PolygonSimplifier(LibElemType& libraryElement)
  : mLibraryElement(libraryElement) {
}

template <typename LibElemType>
PolygonSimplifier<LibElemType>::~PolygonSimplifier() {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

template <typename LibElemType>
void PolygonSimplifier<LibElemType>::convertLineRectsToPolygonRects(
    bool fillArea, bool isGrabArea) noexcept {
  QList<Polygon*> lines;
  while (findLineRectangle(lines)) {
    QSet<LengthBase_t> xValues, yValues;
    foreach (const Polygon* line, lines) {
      xValues.insert(
          line->getPath().getVertices().at(0).getPos().getX().toNm());
      xValues.insert(
          line->getPath().getVertices().at(1).getPos().getX().toNm());
      yValues.insert(
          line->getPath().getVertices().at(0).getPos().getY().toNm());
      yValues.insert(
          line->getPath().getVertices().at(1).getPos().getY().toNm());
    }
    if (xValues.count() != 2 || yValues.count() != 2) break;
    // Q_ASSERT(xValues.count() == 2 && yValues.count() == 2);
    Point p1(xValues.values().first(), yValues.values().first());
    Point p2(xValues.values().first(), yValues.values().last());
    Point p3(xValues.values().last(), yValues.values().last());
    Point p4(xValues.values().last(), yValues.values().first());

    // create the new polygon
    GraphicsLayerName layerName = lines.first()->getLayerName();
    UnsignedLength    lineWidth = lines.first()->getLineWidth();
    Path rectPath({Vertex(p1), Vertex(p2), Vertex(p3), Vertex(p4), Vertex(p1)});
    mLibraryElement.getPolygons().append(
        std::make_shared<Polygon>(Uuid::createRandom(), layerName, lineWidth,
                                  fillArea, isGrabArea, rectPath));

    // remove all lines
    foreach (Polygon* line, lines)
      mLibraryElement.getPolygons().remove(line);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename LibElemType>
bool PolygonSimplifier<LibElemType>::findLineRectangle(
    QList<Polygon*>& lines) noexcept {
  // find lines
  QList<Polygon*> linePolygons;
  for (Polygon& polygon : mLibraryElement.getPolygons()) {
    if (polygon.getPath().getVertices().count() == 2)
      linePolygons.append(&polygon);
  }

  // find rectangle
  Polygon*       line;
  UnsignedLength width(0);
  for (int i = 0; i < linePolygons.count(); i++) {
    lines.clear();
    Point p = linePolygons.at(i)->getPath().getVertices().first().getPos();
    if (findHLine(linePolygons, p, nullptr, &line)) {
      lines.append(line);
      width = line->getLineWidth();
      if (findVLine(linePolygons, p, &width, &line)) {
        lines.append(line);
        if (findHLine(linePolygons, p, &width, &line)) {
          lines.append(line);
          if (findVLine(linePolygons, p, &width, &line)) {
            lines.append(line);
            return true;
          }
        }
      }
    }
  }

  lines.clear();
  return false;
}

template <typename LibElemType>
bool PolygonSimplifier<LibElemType>::findHLine(const QList<Polygon*>& lines,
                                               Point& p, UnsignedLength* width,
                                               Polygon** line) noexcept {
  foreach (Polygon* polygon, lines) {
    if (width) {
      if (polygon->getLineWidth() != *width) continue;
    }
    Point p1 = polygon->getPath().getVertices().at(0).getPos();
    Point p2 = polygon->getPath().getVertices().at(1).getPos();
    if ((p1 == p) && (p2.getY() == p.getY())) {
      *line = polygon;
      p     = p2;
      return true;
    } else if ((p2 == p) && (p1.getY() == p.getY())) {
      *line = polygon;
      p     = p1;
      return true;
    }
  }
  return false;
}

template <typename LibElemType>
bool PolygonSimplifier<LibElemType>::findVLine(const QList<Polygon*>& lines,
                                               Point& p, UnsignedLength* width,
                                               Polygon** line) noexcept {
  foreach (Polygon* polygon, lines) {
    if (width) {
      if (polygon->getLineWidth() != *width) continue;
    }
    Point p1 = polygon->getPath().getVertices().at(0).getPos();
    Point p2 = polygon->getPath().getVertices().at(1).getPos();
    if ((p1 == p) && (p2.getX() == p.getX())) {
      *line = polygon;
      p     = p2;
      return true;
    } else if ((p2 == p) && (p1.getX() == p.getX())) {
      *line = polygon;
      p     = p1;
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Explicit template class instantiation
 ******************************************************************************/

template class PolygonSimplifier<library::Symbol>;
template class PolygonSimplifier<library::Footprint>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
