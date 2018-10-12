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

#ifndef LIBREPCB_PROJECT_SGI_NETPOINT_H
#define LIBREPCB_PROJECT_SGI_NETPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "sgi_base.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class SI_NetPoint;

/*******************************************************************************
 *  Class SGI_NetPoint
 ******************************************************************************/

/**
 * @brief The SGI_NetPoint class
 */
class SGI_NetPoint final : public SGI_Base {
public:
  // Constructors / Destructor
  explicit SGI_NetPoint(SI_NetPoint& netpoint) noexcept;
  ~SGI_NetPoint() noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const { return sBoundingRect; }
  void   paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget);

private:
  // make some methods inaccessible...
  SGI_NetPoint()                          = delete;
  SGI_NetPoint(const SGI_NetPoint& other) = delete;
  SGI_NetPoint& operator=(const SGI_NetPoint& rhs) = delete;

  // Private Methods
  GraphicsLayer* getLayer(const QString& name) const noexcept;

  // General Attributes
  SI_NetPoint&   mNetPoint;
  GraphicsLayer* mLayer;

  // Cached Attributes
  bool mIsVisibleJunction;
  bool mIsOpenLineEnd;

  // Static Stuff
  static QRectF sBoundingRect;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SGI_NETPOINT_H
