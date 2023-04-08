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
#include "bgi_stroketext.h"

#include "../../../graphics/linegraphicsitem.h"
#include "../../../graphics/origincrossgraphicsitem.h"
#include "../../../graphics/primitivepathgraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/project/board/items/bi_device.h>
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

BGI_StrokeText::BGI_StrokeText(BI_StrokeText& text,
                               std::weak_ptr<BGI_Device> deviceItem,
                               const IF_GraphicsLayerProvider& lp) noexcept
  : QGraphicsItemGroup(),
    mText(text),
    mDeviceGraphicsItem(deviceItem),
    mLayerProvider(lp),
    mPathGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mOriginCrossGraphicsItem(new OriginCrossGraphicsItem(this)),
    mAnchorGraphicsItem(new LineGraphicsItem()),
    mOnEditedSlot(*this, &BGI_StrokeText::strokeTextEdited),
    mOnDeviceEditedSlot(*this, &BGI_StrokeText::deviceGraphicsItemEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));

  updatePosition();
  updateTransform();
  updateLayer();
  updateStrokeWidth();
  updatePaths();
  updateAnchorLayer();
  updateAnchorLine();

  mText.onEdited.attach(mOnEditedSlot);
  if (auto ptr = deviceItem.lock()) {
    ptr->onEdited.attach(mOnDeviceEditedSlot);
  }
}

BGI_StrokeText::~BGI_StrokeText() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_StrokeText::shape() const noexcept {
  Q_ASSERT(mPathGraphicsItem && mOriginCrossGraphicsItem);
  return mPathGraphicsItem->mapToParent(mPathGraphicsItem->shape()) |
      mOriginCrossGraphicsItem->shape();
}

QVariant BGI_StrokeText::itemChange(GraphicsItemChange change,
                                    const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mPathGraphicsItem &&
      mOriginCrossGraphicsItem && mAnchorGraphicsItem) {
    mPathGraphicsItem->setSelected(value.toBool());
    mOriginCrossGraphicsItem->setSelected(value.toBool());
    mAnchorGraphicsItem->setSelected(value.toBool());
    updateAnchorLayer();
  } else if ((change == ItemSceneHasChanged) && mAnchorGraphicsItem) {
    if (QGraphicsScene* s = mAnchorGraphicsItem->scene()) {
      s->removeItem(mAnchorGraphicsItem.data());
    }
    if (QGraphicsScene* s = scene()) {
      s->addItem(mAnchorGraphicsItem.data());
    }
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_StrokeText::strokeTextEdited(const BI_StrokeText& obj,
                                      BI_StrokeText::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_StrokeText::Event::PositionChanged:
      updatePosition();
      updateAnchorLine();
      break;
    case BI_StrokeText::Event::RotationChanged:
    case BI_StrokeText::Event::MirroredChanged:
      updateTransform();
      break;
    case BI_StrokeText::Event::LayerChanged:
      updateLayer();
      updateAnchorLayer();
      break;
    case BI_StrokeText::Event::StrokeWidthChanged:
      updateStrokeWidth();
      break;
    case BI_StrokeText::Event::PathsChanged:
      updatePaths();
      break;
    default:
      qWarning()
          << "Unhandled switch-case in BGI_StrokeText::strokeTextEdited():"
          << static_cast<int>(event);
      break;
  }
}

void BGI_StrokeText::deviceGraphicsItemEdited(
    const BGI_Device& obj, BGI_Device::Event event) noexcept {
  switch (event) {
    case BGI_Device::Event::PositionChanged: {
      updateAnchorLine();
      break;
    }
    case BGI_Device::Event::SelectionChanged: {
      setSelected(obj.isSelected());
      break;
    }
    default: { break; }
  }
}

void BGI_StrokeText::updatePosition() noexcept {
  setPos(mText.getData().getPosition().toPxQPointF());
}

void BGI_StrokeText::updateTransform() noexcept {
  QTransform t;
  if (mText.getData().getMirrored()) t.scale(qreal(-1), qreal(1));
  t.rotate(-mText.getData().getRotation().toDeg());
  setTransform(t);
}

void BGI_StrokeText::updateLayer() noexcept {
  Q_ASSERT(mPathGraphicsItem && mOriginCrossGraphicsItem &&
           mAnchorGraphicsItem);

  // Update z-value.
  BoardGraphicsScene::ItemZValue zValue = BoardGraphicsScene::ZValue_Texts;
  if (mText.getData().getLayer().isTop()) {
    zValue = BoardGraphicsScene::ZValue_TextsTop;
  } else if (mText.getData().getLayer().isBottom()) {
    zValue = BoardGraphicsScene::ZValue_TextsBottom;
  }
  setZValue(static_cast<qreal>(zValue));
  mAnchorGraphicsItem->setZValue(static_cast<qreal>(zValue));

  std::shared_ptr<GraphicsLayer> layer =
      mLayerProvider.getLayer(mText.getData().getLayer());
  mPathGraphicsItem->setLineLayer(layer);
  mOriginCrossGraphicsItem->setLayer(layer);
}

void BGI_StrokeText::updateStrokeWidth() noexcept {
  mPathGraphicsItem->setLineWidth(mText.getData().getStrokeWidth());
}

void BGI_StrokeText::updatePaths() noexcept {
  Q_ASSERT(mPathGraphicsItem);
  mPathGraphicsItem->setPath(Path::toQPainterPathPx(mText.getPaths(), false));
}

void BGI_StrokeText::updateAnchorLayer() noexcept {
  Q_ASSERT(mAnchorGraphicsItem);
  if (mText.getDevice() && isSelected()) {
    mAnchorGraphicsItem->setLayer(
        mLayerProvider.getLayer(mText.getData().getLayer()));
  } else {
    mAnchorGraphicsItem->setLayer(nullptr);
  }
}

void BGI_StrokeText::updateAnchorLine() noexcept {
  Q_ASSERT(mAnchorGraphicsItem);
  if (BI_Device* device = mText.getDevice()) {
    mAnchorGraphicsItem->setLine(mText.getData().getPosition(),
                                 device->getPosition());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
