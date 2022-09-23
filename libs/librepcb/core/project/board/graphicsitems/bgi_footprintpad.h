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

#ifndef LIBREPCB_CORE_BGI_FOOTPRINTPAD_H
#define LIBREPCB_CORE_BGI_FOOTPRINTPAD_H

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

class BI_FootprintPad;
class FootprintPad;
class GraphicsLayer;
class PackagePad;

/*******************************************************************************
 *  Class BGI_FootprintPad
 ******************************************************************************/

/**
 * @brief The BGI_FootprintPad class
 */
class BGI_FootprintPad final : public BGI_Base {
public:
  // Constructors / Destructor
  BGI_FootprintPad() = delete;
  BGI_FootprintPad(const BGI_FootprintPad& other) = delete;
  explicit BGI_FootprintPad(BI_FootprintPad& pad) noexcept;
  ~BGI_FootprintPad() noexcept;

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
  BGI_FootprintPad& operator=(const BGI_FootprintPad& rhs) = delete;

private:
  // Private Methods
  GraphicsLayer* getLayer(QString name) const noexcept;

  // General Attributes
  BI_FootprintPad& mPad;
  const FootprintPad& mLibPad;

  // Cached Attributes
  GraphicsLayer* mPadLayer;
  GraphicsLayer* mTopStopMaskLayer;
  GraphicsLayer* mBottomStopMaskLayer;
  GraphicsLayer* mTopCreamMaskLayer;
  GraphicsLayer* mBottomCreamMaskLayer;
  QPainterPath mShape;
  QPainterPath mCopper;
  QPainterPath mStopMask;
  QPainterPath mCreamMask;
  QRectF mBoundingRect;
  QFont mFont;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
