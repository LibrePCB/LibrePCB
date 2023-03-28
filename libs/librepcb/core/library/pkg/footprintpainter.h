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

#ifndef LIBREPCB_CORE_FOOTPRINTPAINTER_H
#define LIBREPCB_CORE_FOOTPRINTPAINTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../export/graphicsexport.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class Footprint;
class FootprintPad;
class Hole;
class Polygon;
class StrokeFont;
class StrokeText;
class Text;

/*******************************************************************************
 *  Class FootprintPainter
 ******************************************************************************/

/**
 * @brief Paints a ::librepcb::Footprint to a QPainter
 *
 * Used for ::librepcb::GraphicsExport.
 */
class FootprintPainter final : public GraphicsPagePainter {
  struct ColorContent {
    QList<QPainterPath> areas;
    QList<Polygon> polygons;
    QList<Circle> circles;
    QList<Hole> holes;
    QList<Hole> padHoles;
    QList<Text> texts;
  };

public:
  // Constructors / Destructor
  FootprintPainter() = delete;
  explicit FootprintPainter(const Footprint& footprint) noexcept;
  FootprintPainter(const FootprintPainter& other) = delete;
  ~FootprintPainter() noexcept;

  // General Methods
  void paint(QPainter& painter, const GraphicsExportSettings& settings) const
      noexcept override;

  // Operator Overloadings
  FootprintPainter& operator=(const FootprintPainter& rhs) = delete;

private:  // Methods
  void initContentByColor() const noexcept;

private:  // Data
  QFont mMonospaceFont;
  const StrokeFont& mStrokeFont;

  QList<FootprintPad> mPads;
  QList<Polygon> mPolygons;
  QList<Circle> mCircles;
  QList<StrokeText> mStrokeTexts;
  QList<Hole> mHoles;

  mutable QMutex mMutex;
  mutable QHash<QString, ColorContent> mContentByColor;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
