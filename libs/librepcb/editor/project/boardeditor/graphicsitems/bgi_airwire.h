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

#ifndef LIBREPCB_EDITOR_BGI_AIRWIRE_H
#define LIBREPCB_EDITOR_BGI_AIRWIRE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicslayer.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_AirWire;
class NetSignal;

namespace editor {

class GraphicsLayer;
class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class BGI_AirWire
 ******************************************************************************/

/**
 * @brief The BGI_AirWire class
 */
class BGI_AirWire final : public QGraphicsItem {
public:
  // Constructors / Destructor
  BGI_AirWire() = delete;
  BGI_AirWire(const BGI_AirWire& other) = delete;
  BGI_AirWire(BI_AirWire& airwire, const IF_GraphicsLayerProvider& lp,
              std::shared_ptr<const QSet<const NetSignal*>>
                  highlightedNetSignals) noexcept;
  virtual ~BGI_AirWire() noexcept;

  // General Methods
  BI_AirWire& getAirWire() noexcept { return mAirWire; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const { return mBoundingRect; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget);

  // Operator Overloadings
  BGI_AirWire& operator=(const BGI_AirWire& rhs) = delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;

private:  // Data
  BI_AirWire& mAirWire;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  std::shared_ptr<GraphicsLayer> mLayer;

  // Cached Attributes
  QVector<QLineF> mLines;
  QRectF mBoundingRect;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
