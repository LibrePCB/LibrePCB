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

#ifndef LIBREPCB_CORE_SGI_NETLABEL_H
#define LIBREPCB_CORE_SGI_NETLABEL_H

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
class LineGraphicsItem;
class SI_NetLabel;

/*******************************************************************************
 *  Class SGI_NetLabel
 ******************************************************************************/

/**
 * @brief The SGI_NetLabel class
 */
class SGI_NetLabel final : public SGI_Base {
public:
  // Constructors / Destructor
  SGI_NetLabel() = delete;
  SGI_NetLabel(const SGI_NetLabel& other) = delete;
  explicit SGI_NetLabel(SI_NetLabel& netlabel) noexcept;
  ~SGI_NetLabel() noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;
  void setAnchor(const Point& pos) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const { return mBoundingRect; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget);

  // Operator Overloadings
  SGI_NetLabel& operator=(const SGI_NetLabel& rhs) = delete;

private:
  // Private Methods
  GraphicsLayer* getLayer(const QString& name) const noexcept;

  // Attributes
  SI_NetLabel& mNetLabel;
  QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;

  // Cached Attributes
  QStaticText mStaticText;
  QFont mFont;
  bool mRotate180;
  QPointF mTextOrigin;
  QRectF mBoundingRect;

  // Static Stuff
  static QVector<QLineF> sOriginCrossLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
