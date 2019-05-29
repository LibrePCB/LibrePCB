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

#ifndef LIBREPCB_PROJECT_BGI_PLANE_H
#define LIBREPCB_PROJECT_BGI_PLANE_H

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

class Path;
class Polygon;
class GraphicsLayer;

namespace project {

class BI_Plane;

/*******************************************************************************
 *  Class BGI_Plane
 ******************************************************************************/

/**
 * @brief The BGI_Plane class
 */
class BGI_Plane final : public BGI_Base {
public:
  // Constructors / Destructor
  explicit BGI_Plane(BI_Plane& plane) noexcept;
  ~BGI_Plane() noexcept;

  // Getters
  bool isSelectable() const noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF       boundingRect() const noexcept { return mBoundingRect; }
  QPainterPath shape() const noexcept { return mShape; }
  void         paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                     QWidget* widget = 0);

private:
  // make some methods inaccessible...
  BGI_Plane()                       = delete;
  BGI_Plane(const BGI_Plane& other) = delete;
  BGI_Plane& operator=(const BGI_Plane& rhs) = delete;

  // Private Methods
  GraphicsLayer* getLayer(QString name) const noexcept;

  // General Attributes
  BI_Plane& mPlane;

  // Cached Attributes
  GraphicsLayer*        mLayer;
  QRectF                mBoundingRect;
  QPainterPath          mShape;
  QPainterPath          mOutline;
  QVector<QPainterPath> mAreas;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BGI_PLANE_H
