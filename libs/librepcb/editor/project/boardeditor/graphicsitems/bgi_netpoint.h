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

#ifndef LIBREPCB_EDITOR_BGI_NETPOINT_H
#define LIBREPCB_EDITOR_BGI_NETPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicslayer.h"

#include <librepcb/core/project/board/items/bi_netpoint.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class BGI_NetPoint
 ******************************************************************************/

/**
 * @brief The BGI_NetPoint class
 */
class BGI_NetPoint final : public QGraphicsItem {
public:
  // Constructors / Destructor
  BGI_NetPoint() = delete;
  BGI_NetPoint(const BGI_NetPoint& other) = delete;
  BGI_NetPoint(BI_NetPoint& netpoint,
               const IF_GraphicsLayerProvider& lp) noexcept;
  virtual ~BGI_NetPoint() noexcept;

  // General Methods
  BI_NetPoint& getNetPoint() noexcept { return mNetPoint; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) noexcept override;

  // Operator Overloadings
  BGI_NetPoint& operator=(const BGI_NetPoint& rhs) = delete;

private:  // Methods
  void netPointEdited(const BI_NetPoint& obj,
                      BI_NetPoint::Event event) noexcept;
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateLayer() noexcept;
  void updatePosition() noexcept;
  void updateDiameter() noexcept;
  void updateNetSignalName() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  // General Attributes
  BI_NetPoint& mNetPoint;
  const IF_GraphicsLayerProvider& mLayerProvider;
  GraphicsLayer* mLayer;

  // Cached Attributes
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  BI_NetPoint::OnEditedSlot mOnEditedSlot;
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
