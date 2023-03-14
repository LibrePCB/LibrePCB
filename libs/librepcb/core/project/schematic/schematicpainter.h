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

#ifndef LIBREPCB_CORE_SCHEMATICPAINTER_H
#define LIBREPCB_CORE_SCHEMATICPAINTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../export/graphicsexport.h"
#include "../../types/alignment.h"
#include "../../types/length.h"
#include "../../utils/transform.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class Path;
class Polygon;
class Schematic;
class Text;

/*******************************************************************************
 *  Class SchematicPainter
 ******************************************************************************/

/**
 * @brief Paints a ::librepcb::Schematic to a QPainter
 *
 * Used for ::librepcb::GraphicsExport.
 */
class SchematicPainter final : public GraphicsPagePainter {
  struct Pin {
    Point position;
    Angle rotation;
    UnsignedLength length;
    QString name;
    Point namePosition;
    Angle nameRotation;
    PositiveLength nameHeight;
    Alignment nameAlignment;
  };

  struct Line {
    Point startPosition;
    Point endPosition;
    UnsignedLength width;
  };

  struct Label {
    Point position;
    Angle rotation;
    bool mirrored;
    QString text;
  };

  struct Symbol {
    Transform transform;
    QList<Pin> pins;
    QList<Polygon> polygons;
    QList<Circle> circles;
  };

public:
  // Constructors / Destructor
  SchematicPainter() = delete;
  explicit SchematicPainter(const Schematic& schematic,
                            bool thumbnail = false) noexcept;
  SchematicPainter(const SchematicPainter& other) = delete;
  ~SchematicPainter() noexcept;

  // General Methods
  void paint(QPainter& painter, const GraphicsExportSettings& settings) const
      noexcept override;

  // Operator Overloadings
  SchematicPainter& operator=(const SchematicPainter& rhs) = delete;

private:  // Data
  QList<Symbol> mSymbols;
  QList<Point> mJunctions;
  QList<Line> mNetLines;
  QList<Label> mNetLabels;
  QList<Polygon> mPolygons;
  QList<Text> mTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
