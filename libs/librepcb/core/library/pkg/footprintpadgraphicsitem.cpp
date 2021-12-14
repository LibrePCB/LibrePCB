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
#include "footprintpadgraphicsitem.h"

#include "../../graphics/graphicslayer.h"
#include "../../graphics/primitivepathgraphicsitem.h"
#include "../../graphics/primitivetextgraphicsitem.h"
#include "../../types/angle.h"
#include "../../types/length.h"
#include "../../types/point.h"
#include "footprintpad.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintPadGraphicsItem::FootprintPadGraphicsItem(
    FootprintPad& pad, const IF_GraphicsLayerProvider& lp,
    const PackagePadList* packagePadList, QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mPad(pad),
    mLayerProvider(lp),
    mPackagePadList(packagePadList),
    mPathGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOnPadsEditedSlot(*this, &FootprintPadGraphicsItem::packagePadListEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, false);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(10);

  // path properties
  mPathGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

  // text properties
  mTextGraphicsItem->setHeight(PositiveLength(1000000));
  mTextGraphicsItem->setAlignment(
      Alignment(HAlign::center(), VAlign::center()));

  // pad properties
  setPosition(mPad.getPosition());
  setRotation(mPad.getRotation());
  setShape(mPad.toQPainterPathPx());
  setLayerName(mPad.getLayerName());
  setPackagePadUuid(mPad.getPackagePadUuid());

  // register to the pad to get attribute updates
  mPad.registerGraphicsItem(*this);

  // register to the package pad list to get notified about updates
  if (mPackagePadList) {
    mPackagePadList->onEdited.attach(mOnPadsEditedSlot);
  }
}

FootprintPadGraphicsItem::~FootprintPadGraphicsItem() noexcept {
  mPad.unregisterGraphicsItem(*this);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPadGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void FootprintPadGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void FootprintPadGraphicsItem::setShape(const QPainterPath& shape) noexcept {
  mPathGraphicsItem->setPath(shape);
  updateTextHeight();
}

void FootprintPadGraphicsItem::setLayerName(const QString& name) noexcept {
  mPathGraphicsItem->setFillLayer(mLayerProvider.getLayer(name));
  mTextGraphicsItem->setLayer(mLayerProvider.getLayer(name));
}

void FootprintPadGraphicsItem::setPackagePadUuid(
    const tl::optional<Uuid>& uuid) noexcept {
  QString name;
  if (mPackagePadList && uuid) {
    if (std::shared_ptr<const PackagePad> pad = mPackagePadList->find(*uuid)) {
      name = *pad->getName();
    }
  }
  mTextGraphicsItem->setText(name);
  updateTextHeight();
}

void FootprintPadGraphicsItem::setSelected(bool selected) noexcept {
  mPathGraphicsItem->setSelected(selected);
  mTextGraphicsItem->setSelected(selected);
  QGraphicsItem::setSelected(selected);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath FootprintPadGraphicsItem::shape() const noexcept {
  return mPathGraphicsItem->shape();
}

void FootprintPadGraphicsItem::paint(QPainter* painter,
                                     const QStyleOptionGraphicsItem* option,
                                     QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintPadGraphicsItem::packagePadListEdited(
    const PackagePadList& list, int index,
    const std::shared_ptr<const PackagePad>& pad,
    PackagePadList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(pad);
  Q_UNUSED(event);
  setPackagePadUuid(mPad.getPackagePadUuid());
}

void FootprintPadGraphicsItem::updateTextHeight() noexcept {
  QRectF padRect = mPathGraphicsItem->boundingRect();
  QRectF textRect = mTextGraphicsItem->boundingRect();
  qreal heightRatio = textRect.height() / padRect.height();
  qreal widthRatio = textRect.width() / padRect.width();
  qreal ratio = qMax(heightRatio, widthRatio);
  mTextGraphicsItem->setScale(1.0 / ratio);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
