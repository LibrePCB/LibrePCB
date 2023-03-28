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

#include "../../graphics/primitivefootprintpadgraphicsitem.h"

#include <librepcb/core/types/layer.h>
#include <librepcb/core/workspace/theme.h>

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
  : QGraphicsItemGroup(parent),
    mPad(pad),
    mPackagePadList(packagePadList),
    mGraphicsItem(new PrimitiveFootprintPadGraphicsItem(lp, true, this)),
    mOnPadEditedSlot(*this, &FootprintPadGraphicsItem::padEdited),
    mOnPackagePadsEditedSlot(*this,
                             &FootprintPadGraphicsItem::packagePadListEdited) {
  Q_ASSERT(mPad);

  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(10);

  setPos(mPad->getPosition().toPxQPointF());
  mGraphicsItem->setRotation(mPad->getRotation());
  updateLayer();
  updateGeometries();
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

void FootprintPadGraphicsItem::updateText() noexcept {
  QString text;
  if (mPackagePadList && mPad->getPackagePadUuid()) {
    if (std::shared_ptr<const PackagePad> pad =
            mPackagePadList->find(*mPad->getPackagePadUuid())) {
      text = *pad->getName();
    }
  }
  mGraphicsItem->setText(text);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath FootprintPadGraphicsItem::shape() const noexcept {
  Q_ASSERT(mGraphicsItem);
  return mGraphicsItem->mapToParent(mGraphicsItem->shape());
}

QVariant FootprintPadGraphicsItem::itemChange(GraphicsItemChange change,
                                              const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mGraphicsItem) {
    mGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
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
      setPos(pad.getPosition().toPxQPointF());
      break;
    case FootprintPad::Event::RotationChanged:
      mGraphicsItem->setRotation(pad.getRotation());
      break;
    case FootprintPad::Event::ShapeChanged:
    case FootprintPad::Event::WidthChanged:
    case FootprintPad::Event::HeightChanged:
    case FootprintPad::Event::RadiusChanged:
    case FootprintPad::Event::CustomShapeOutlineChanged:
      updateGeometries();
      break;
    case FootprintPad::Event::StopMaskConfigChanged:
    case FootprintPad::Event::SolderPasteConfigChanged:
      updateGeometries();
      break;
    case FootprintPad::Event::ComponentSideChanged:
    case FootprintPad::Event::HolesEdited:
      updateLayer();
      updateGeometries();
      break;
    default:
      qWarning()
          << "Unhandled switch-case in FootprintPadGraphicsItem::padEdited():"
          << static_cast<int>(event);
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

void FootprintPadGraphicsItem::updateLayer() noexcept {
  mGraphicsItem->setLayer(mPad->isTht() ? Theme::Color::sBoardPads
                                        : mPad->getSmtLayer().getThemeColor());
}

void FootprintPadGraphicsItem::updateGeometries() noexcept {
  const PadGeometry geometry = mPad->getGeometry();
  const Length stopMaskOffset = mPad->getStopMaskConfig().getOffset()
      ? *mPad->getStopMaskConfig().getOffset()
      : Length(100000);
  const Length solderPasteOffset = mPad->getSolderPasteConfig().getOffset()
      ? *mPad->getSolderPasteConfig().getOffset()
      : Length(100000);

  QHash<const Layer*, QList<PadGeometry>> geometries;
  if (mPad->hasTopCopper()) {
    geometries.insert(&Layer::topCopper(), {geometry});
  }
  if (mPad->hasAutoTopStopMask()) {
    geometries.insert(&Layer::topStopMask(),
                      {geometry.withOffset(stopMaskOffset)});
  }
  if (mPad->hasAutoTopSolderPaste()) {
    geometries.insert(&Layer::topSolderPaste(),
                      {geometry.withOffset(-solderPasteOffset)});
  }
  if (mPad->hasBottomCopper()) {
    geometries.insert(&Layer::botCopper(), {geometry});
  }
  if (mPad->hasAutoBottomStopMask()) {
    geometries.insert(&Layer::botStopMask(),
                      {geometry.withOffset(stopMaskOffset)});
  }
  if (mPad->hasAutoBottomSolderPaste()) {
    geometries.insert(&Layer::botSolderPaste(),
                      {geometry.withOffset(-solderPasteOffset)});
  }
  mGraphicsItem->setGeometries(geometries);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
