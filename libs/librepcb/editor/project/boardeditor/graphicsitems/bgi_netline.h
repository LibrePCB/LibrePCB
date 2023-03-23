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

#ifndef LIBREPCB_EDITOR_BGI_NETLINE_H
#define LIBREPCB_EDITOR_BGI_NETLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicslayer.h"

#include <librepcb/core/project/board/items/bi_netline.h>

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
 *  Class BGI_NetLine
 ******************************************************************************/

/**
 * @brief The BGI_NetLine class
 */
class BGI_NetLine final : public QGraphicsItem {
public:
  // Constructors / Destructor
  BGI_NetLine() = delete;
  BGI_NetLine(const BGI_NetLine& other) = delete;
  BGI_NetLine(BI_NetLine& netline, const IF_GraphicsLayerProvider& lp,
              std::shared_ptr<const QSet<const NetSignal*>>
                  highlightedNetSignals) noexcept;
  virtual ~BGI_NetLine() noexcept;

  // General Methods
  BI_NetLine& getNetLine() noexcept { return mNetLine; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) noexcept override;

  // Operator Overloadings
  BGI_NetLine& operator=(const BGI_NetLine& rhs) = delete;

private:  // Methods
  void netLineEdited(const BI_NetLine& obj, BI_NetLine::Event event) noexcept;
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateLine() noexcept;
  void updateLayer() noexcept;
  void updateNetSignalName() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  // Attributes
  BI_NetLine& mNetLine;
  const IF_GraphicsLayerProvider& mLayerProvider;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  GraphicsLayer* mLayer;

  // Cached Attributes
  QLineF mLineF;
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  BI_NetLine::OnEditedSlot mOnNetLineEditedSlot;
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
