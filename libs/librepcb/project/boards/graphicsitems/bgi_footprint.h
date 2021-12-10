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

#ifndef LIBREPCB_PROJECT_BGI_FOOTPRINT_H
#define LIBREPCB_PROJECT_BGI_FOOTPRINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bgi_base.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;
class StrokeText;

namespace library {
class Footprint;
}

namespace project {

class BI_Footprint;

/*******************************************************************************
 *  Class BGI_Footprint
 ******************************************************************************/

/**
 * @brief The BGI_Footprint class
 */
class BGI_Footprint final : public BGI_Base {
public:
  // Constructors / Destructor
  BGI_Footprint() = delete;
  BGI_Footprint(const BGI_Footprint& other) = delete;
  explicit BGI_Footprint(BI_Footprint& footprint) noexcept;
  ~BGI_Footprint() noexcept;

  // Getters
  bool isSelectable() const noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept { return mBoundingRect; }
  QPainterPath shape() const noexcept { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0);

  // Operator Overloadings
  BGI_Footprint& operator=(const BGI_Footprint& rhs) = delete;

private:
  // Private Methods
  GraphicsLayer* getLayer(QString name) const noexcept;

  // General Attributes
  BI_Footprint& mFootprint;
  const library::Footprint& mLibFootprint;

  // Cached Attributes
  QRectF mBoundingRect;
  QPainterPath mShape;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
