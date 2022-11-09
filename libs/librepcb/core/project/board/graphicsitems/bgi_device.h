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

#ifndef LIBREPCB_CORE_BGI_DEVICE_H
#define LIBREPCB_CORE_BGI_DEVICE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicslayer.h"
#include "bgi_base.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class GraphicsLayer;
class OriginCrossGraphicsItem;
class PrimitiveCircleGraphicsItem;
class PrimitivePathGraphicsItem;

/*******************************************************************************
 *  Class BGI_Device
 ******************************************************************************/

/**
 * @brief The BGI_Device class
 */
class BGI_Device final : public BGI_Base {
public:
  // Constructors / Destructor
  BGI_Device() = delete;
  BGI_Device(const BGI_Device& other) = delete;
  explicit BGI_Device(BI_Device& device) noexcept;
  ~BGI_Device() noexcept;

  // General Methods
  bool isSelectable() const noexcept;
  void setSelected(bool selected) noexcept;
  void updateBoardSide() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept { return mBoundingRect; }
  QPainterPath shape() const noexcept;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0);

  // Operator Overloadings
  BGI_Device& operator=(const BGI_Device& rhs) = delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  GraphicsLayer* getLayer(QString name) const noexcept;

private:  // Data
  BI_Device& mDevice;
  QPointer<GraphicsLayer> mGrabAreaLayer;
  std::shared_ptr<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  QVector<std::shared_ptr<PrimitiveCircleGraphicsItem>> mCircleGraphicsItems;
  QVector<std::shared_ptr<PrimitivePathGraphicsItem>> mPolygonGraphicsItems;
  QVector<std::shared_ptr<PrimitiveCircleGraphicsItem>> mHoleGraphicsItems;
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
