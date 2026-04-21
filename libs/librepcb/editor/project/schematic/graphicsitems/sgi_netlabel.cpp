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
#include "sgi_netlabel.h"

#include "../../../graphics/graphicslayerlist.h"
#include "../../../graphics/linegraphicsitem.h"
#include "../../../graphics/origincrossgraphicsitem.h"
#include "../../../graphics/primitivetextgraphicsitem.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/workspace/colorrole.h>

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

SGI_NetLabel::SGI_NetLabel(
    SI_NetLabel& label, const GraphicsLayerList& layers,
    std::shared_ptr<const SchematicGraphicsScene::Context> context) noexcept
  : QGraphicsItemGroup(),
    mLabel(label),
    mContext(context),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOriginCrossGraphicsItem(new OriginCrossGraphicsItem(this)),
    mAnchorGraphicsItem(new LineGraphicsItem()),
    mOnEditedSlot(*this, &SGI_NetLabel::labelEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_NetLabels);

  mTextGraphicsItem->setLayer(layers.get(ColorRole::schematicNetLabels()));
  mTextGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::Monospace);
  mTextGraphicsItem->setHeight(PositiveLength(Length::fromPx(4)));
  mTextGraphicsItem->setLevelOfDetailToPixelate(6);
  mTextGraphicsItem->setLevelOfDetailToHide(2);
  mTextGraphicsItem->setSelected(isSelected());

  mOriginCrossGraphicsItem->setLayer(
      layers.get(ColorRole::schematicReferences()));
  mOriginCrossGraphicsItem->setSize(UnsignedLength(800000));
  mOriginCrossGraphicsItem->setSelected(isSelected());

  mAnchorGraphicsItem->setLayer(layers.get(ColorRole::schematicReferences()));
  mAnchorGraphicsItem->setZValue(SchematicGraphicsScene::ZValue_NetLabels);
  mAnchorGraphicsItem->setSelected(isSelected());

  updateContext();
  updatePosition();
  updateRotation();
  updateMirrored();
  updateText();
  updateAnchor();

  mLabel.onEdited.attach(mOnEditedSlot);
}

SGI_NetLabel::~SGI_NetLabel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SGI_NetLabel::updateContext() noexcept {
  const NetSignal* net = &mLabel.getNetSegment().getNetSignal();
  const GraphicsLayer::State state = mContext->getLayerState(false, net);
  mTextGraphicsItem->setState(state);
  mOriginCrossGraphicsItem->setState(state);
  mAnchorGraphicsItem->setState(state);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath SGI_NetLabel::shape() const noexcept {
  return mTextGraphicsItem->mapToParent(mTextGraphicsItem->shape()) |
      mOriginCrossGraphicsItem->shape();
}

QVariant SGI_NetLabel::itemChange(GraphicsItemChange change,
                                  const QVariant& value) noexcept {
  if ((change == ItemSceneHasChanged) && mAnchorGraphicsItem) {
    if (QGraphicsScene* s = mAnchorGraphicsItem->scene()) {
      s->removeItem(mAnchorGraphicsItem.get());
    }
    if (QGraphicsScene* s = scene()) {
      s->addItem(mAnchorGraphicsItem.get());
    }
  } else if ((change == ItemSelectedHasChanged) && mAnchorGraphicsItem) {
    mTextGraphicsItem->setSelected(value.toBool());
    mOriginCrossGraphicsItem->setSelected(value.toBool());
    mAnchorGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_NetLabel::labelEdited(const SI_NetLabel& obj,
                               SI_NetLabel::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_NetLabel::Event::PositionChanged:
      updatePosition();
      updateAnchor();
      break;
    case SI_NetLabel::Event::RotationChanged:
      updateRotation();
      break;
    case SI_NetLabel::Event::MirroredChanged:
      updateMirrored();
      break;
    case SI_NetLabel::Event::NetNameChanged:
      updateText();
      break;
    case SI_NetLabel::Event::AnchorPositionChanged:
      updateAnchor();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_NetLabel::labelEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_NetLabel::updatePosition() noexcept {
  setPos(mLabel.getPosition().toPxQPointF());
}

void SGI_NetLabel::updateRotation() noexcept {
  mTextGraphicsItem->setRotation(mLabel.getRotation());
  mOriginCrossGraphicsItem->setRotation(mLabel.getRotation());
}

void SGI_NetLabel::updateMirrored() noexcept {
  mTextGraphicsItem->setAlignment(
      Alignment(mLabel.getMirrored() ? HAlign::right() : HAlign::left(),
                VAlign::bottom()));
}

void SGI_NetLabel::updateText() noexcept {
  mTextGraphicsItem->setText(*mLabel.getNetSegment().getNetSignal().getName(),
                             true);
}

void SGI_NetLabel::updateAnchor() noexcept {
  mAnchorGraphicsItem->setLine(mLabel.getPosition(),
                               mLabel.getAnchorPosition());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
