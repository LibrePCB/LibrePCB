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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bgi_zone.h"

#include "../../../graphics/primitivezonegraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_Zone::BGI_Zone(BI_Zone& zone, const IF_GraphicsLayerProvider& lp) noexcept
  : QGraphicsItemGroup(),
    mZone(zone),
    mGraphicsItem(new PrimitiveZoneGraphicsItem(lp, this)),
    mOnEditedSlot(*this, &BGI_Zone::zoneEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  mGraphicsItem->setAllLayers(mZone.getBoard().getCopperLayers());
  mGraphicsItem->setEnabledLayers(mZone.getData().getLayers());
  mGraphicsItem->setOutline(mZone.getData().getOutline());
  updateZValue();
  updateEditable();

  mZone.onEdited.attach(mOnEditedSlot);
}

BGI_Zone::~BGI_Zone() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int BGI_Zone::getLineIndexAtPosition(const Point& pos) const noexcept {
  return mGraphicsItem->getLineIndexAtPosition(pos);
}

QVector<int> BGI_Zone::getVertexIndicesAtPosition(
    const Point& pos) const noexcept {
  return mGraphicsItem->getVertexIndicesAtPosition(pos);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_Zone::shape() const noexcept {
  return mGraphicsItem->shape();
}

QVariant BGI_Zone::itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mGraphicsItem) {
    mGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Zone::zoneEdited(const BI_Zone& obj, BI_Zone::Event event) noexcept {
  switch (event) {
    case BI_Zone::Event::BoardLayersChanged:
      mGraphicsItem->setAllLayers(mZone.getBoard().getCopperLayers());
      break;
    case BI_Zone::Event::LayersChanged:
      mGraphicsItem->setEnabledLayers(mZone.getData().getLayers());
      updateZValue();
      break;
    case BI_Zone::Event::RulesChanged:
      break;
    case BI_Zone::Event::OutlineChanged:
      mGraphicsItem->setOutline(obj.getData().getOutline());
      break;
    case BI_Zone::Event::IsLockedChanged:
      updateEditable();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Zone::zoneEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_Zone::updateZValue() noexcept {
  auto layers =
      Toolbox::sortedQSet(mZone.getData().getLayers(), &Layer::lessThan);
  if (!layers.isEmpty()) {
    setZValue(BoardGraphicsScene::getZValueOfCopperLayer(*layers.first()));
  }
}

void BGI_Zone::updateEditable() noexcept {
  mGraphicsItem->setEditable(!mZone.getData().isLocked());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
