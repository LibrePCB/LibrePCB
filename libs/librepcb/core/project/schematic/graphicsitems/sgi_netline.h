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

#ifndef LIBREPCB_CORE_SGI_NETLINE_H
#define LIBREPCB_CORE_SGI_NETLINE_H

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
class SI_NetLine;

/*******************************************************************************
 *  Class SGI_NetLine
 ******************************************************************************/

/**
 * @brief The SGI_NetLine class
 */
class SGI_NetLine final : public SGI_Base {
public:
  // Constructors / Destructor
  SGI_NetLine() = delete;
  SGI_NetLine(const SGI_NetLine& other) = delete;
  explicit SGI_NetLine(SI_NetLine& netline) noexcept;
  ~SGI_NetLine() noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const { return mBoundingRect; }
  QPainterPath shape() const { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget);

  // Operator Overloadings
  SGI_NetLine& operator=(const SGI_NetLine& rhs) = delete;

private:
  // Private Methods
  GraphicsLayer* getLayer(const QString& name) const noexcept;

  // Attributes
  SI_NetLine& mNetLine;
  GraphicsLayer* mLayer;

  // Cached Attributes
  QLineF mLineF;
  QRectF mBoundingRect;
  QPainterPath mShape;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
