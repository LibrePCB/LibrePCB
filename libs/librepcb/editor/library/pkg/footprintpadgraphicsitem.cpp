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

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/graphics/primitivepathgraphicsitem.h>
#include <librepcb/core/graphics/primitivetextgraphicsitem.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

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

FootprintPadGraphicsItem::FootprintPadGraphicsItem(
    std::shared_ptr<FootprintPad> pad, const IF_GraphicsLayerProvider& lp,
    const PackagePadList* packagePadList, QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mPad(pad),
    mLayerProvider(lp),
    mPackagePadList(packagePadList),
    mPathGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOnPadEditedSlot(*this, &FootprintPadGraphicsItem::padEdited),
    mOnPackagePadsEditedSlot(*this,
                             &FootprintPadGraphicsItem::packagePadListEdited) {
  Q_ASSERT(mPad);

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
  setPosition(mPad->getPosition());
  setRotation(mPad->getRotation());
  setShape(mPad->toQPainterPathPx());
  setLayerName(mPad->getLayerName());
  updateText();

  // Register to the pad(s) to get notified about any modifications.
  mPad->onEdited.attach(mOnPadEditedSlot);
  if (mPackagePadList) {
    mPackagePadList->onEdited.attach(mOnPackagePadsEditedSlot);
  }
}

FootprintPadGraphicsItem::~FootprintPadGraphicsItem() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPadGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void FootprintPadGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());

  // Keep the text always at 0° for readability.
  mTextGraphicsItem->setRotation(-rot);
}

void FootprintPadGraphicsItem::setSelected(bool selected) noexcept {
  mPathGraphicsItem->setSelected(selected);
  mTextGraphicsItem->setSelected(selected);
  QGraphicsItem::setSelected(selected);
}

void FootprintPadGraphicsItem::updateText() noexcept {
  QString text;
  if (mPackagePadList && mPad->getPackagePadUuid()) {
    if (std::shared_ptr<const PackagePad> pad =
            mPackagePadList->find(*mPad->getPackagePadUuid())) {
      text = *pad->getName();
    }
  }
  setToolTip(text);
  mTextGraphicsItem->setText(text);
  updateTextHeight();
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

void FootprintPadGraphicsItem::padEdited(const FootprintPad& pad,
                                         FootprintPad::Event event) noexcept {
  switch (event) {
    case FootprintPad::Event::UuidChanged:
      break;
    case FootprintPad::Event::PackagePadUuidChanged:
      updateText();
      break;
    case FootprintPad::Event::PositionChanged:
      setPosition(pad.getPosition());
      break;
    case FootprintPad::Event::RotationChanged:
      setRotation(pad.getRotation());
      break;
    case FootprintPad::Event::ShapeChanged:
    case FootprintPad::Event::WidthChanged:
    case FootprintPad::Event::HeightChanged:
    case FootprintPad::Event::DrillDiameterChanged:
      setShape(pad.toQPainterPathPx());
      break;
    case FootprintPad::Event::BoardSideChanged:
      setLayerName(pad.getLayerName());
      break;
    default:
      qWarning()
          << "Unhandled switch-case in FootprintPadGraphicsItem::padEdited()";
      break;
  }
}

void FootprintPadGraphicsItem::packagePadListEdited(
    const PackagePadList& list, int index,
    const std::shared_ptr<const PackagePad>& pad,
    PackagePadList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(pad);
  Q_UNUSED(event);
  updateText();
}

void FootprintPadGraphicsItem::setShape(const QPainterPath& shape) noexcept {
  mPathGraphicsItem->setPath(shape);
  updateTextHeight();
}

void FootprintPadGraphicsItem::setLayerName(const QString& name) noexcept {
  mPathGraphicsItem->setFillLayer(mLayerProvider.getLayer(name));
  mTextGraphicsItem->setLayer(mLayerProvider.getLayer(name));
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

}  // namespace editor
}  // namespace librepcb
