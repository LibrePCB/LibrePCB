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

#ifndef LIBREPCB_EDITOR_BGI_VIA_H
#define LIBREPCB_EDITOR_BGI_VIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/board/items/bi_via.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;

namespace editor {

/*******************************************************************************
 *  Class BGI_Via
 ******************************************************************************/

/**
 * @brief The BGI_Via class
 */
class BGI_Via final : public QGraphicsItem {
public:
  // Constructors / Destructor
  BGI_Via() = delete;
  BGI_Via(const BGI_Via& other) = delete;
  BGI_Via(BI_Via& via,
          std::shared_ptr<const QSet<const NetSignal*>>
              highlightedNetSignals) noexcept;
  virtual ~BGI_Via() noexcept;

  // General Methods
  BI_Via& getVia() noexcept { return mVia; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) noexcept override;

  // Operator Overloadings
  BGI_Via& operator=(const BGI_Via& rhs) = delete;

private:  // Methods
  void viaEdited(const BI_Via& obj, BI_Via::Event event) noexcept;
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updatePosition() noexcept;
  void updateShapes() noexcept;
  void updateNetSignalName() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  // General Attributes
  BI_Via& mVia;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  GraphicsLayer* mViaLayer;
  GraphicsLayer* mTopStopMaskLayer;
  GraphicsLayer* mBottomStopMaskLayer;

  // Cached Attributes
  bool mDrawStopMask;
  QPainterPath mShape;
  QPainterPath mCopper;
  QPainterPath mStopMask;
  QRectF mBoundingRect;
  QFont mFont;

  // Slots
  BI_Via::OnEditedSlot mOnEditedSlot;
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
