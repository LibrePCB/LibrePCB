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
#include "bgi_footprintpad.h"

#include "../../../graphics/primitivefootprintpadgraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/pkg/packagepad.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
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

BGI_FootprintPad::BGI_FootprintPad(BI_FootprintPad& pad,
                                   std::weak_ptr<BGI_Device> deviceItem,
                                   const GraphicsLayerList& layers,
                                   std::shared_ptr<const QSet<const NetSignal*>>
                                       highlightedNetSignals) noexcept
  : QGraphicsItemGroup(),
    mPad(pad),
    mDeviceGraphicsItem(deviceItem),
    mHighlightedNetSignals(highlightedNetSignals),
    mGraphicsItem(new PrimitiveFootprintPadGraphicsItem(layers, false, this)),
    mOnPadEditedSlot(*this, &BGI_FootprintPad::padEdited),
    mOnDeviceEditedSlot(*this, &BGI_FootprintPad::deviceGraphicsItemEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  setPos(mPad.getPosition().toPxQPointF());
  mGraphicsItem->setRotation(mPad.getRotation());
  mGraphicsItem->setMirrored(mPad.getMirrored());
  mGraphicsItem->setText(mPad.getText());
  mGraphicsItem->setGeometries(mPad.getGeometries(),
                               *mPad.getLibPad().getCopperClearance());
  updateLayer();
  updateToolTip();

  mPad.onEdited.attach(mOnPadEditedSlot);
  if (auto ptr = mDeviceGraphicsItem.lock()) {
    ptr->onEdited.attach(mOnDeviceEditedSlot);
  }
}

BGI_FootprintPad::~BGI_FootprintPad() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_FootprintPad::updateHighlightedNetSignals() noexcept {
  updateHightlighted(isSelected());
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_FootprintPad::shape() const noexcept {
  Q_ASSERT(mGraphicsItem);
  return mGraphicsItem->mapToParent(mGraphicsItem->shape());
}

QVariant BGI_FootprintPad::itemChange(GraphicsItemChange change,
                                      const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mGraphicsItem) {
    updateHightlighted(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_FootprintPad::padEdited(const BI_FootprintPad& obj,
                                 BI_FootprintPad::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_FootprintPad::Event::PositionChanged:
      setPos(obj.getPosition().toPxQPointF());
      break;
    case BI_FootprintPad::Event::RotationChanged:
      mGraphicsItem->setRotation(obj.getRotation());
      break;
    case BI_FootprintPad::Event::MirroredChanged:
      mGraphicsItem->setMirrored(obj.getMirrored());
      updateLayer();
      break;
    case BI_FootprintPad::Event::TextChanged:
      mGraphicsItem->setText(obj.getText());
      updateToolTip();
      break;
    case BI_FootprintPad::Event::GeometriesChanged:
      mGraphicsItem->setGeometries(obj.getGeometries(),
                                   *obj.getLibPad().getCopperClearance());
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_FootprintPad::padEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_FootprintPad::deviceGraphicsItemEdited(
    const BGI_Device& obj, BGI_Device::Event event) noexcept {
  if (event == BGI_Device::Event::SelectionChanged) {
    setSelected(obj.isSelected());
  }
}

void BGI_FootprintPad::updateLayer() noexcept {
  if (mPad.getLibPad().isTht()) {
    setZValue(BoardGraphicsScene::ZValue_FootprintPadsTop);
    mGraphicsItem->setLayer(Theme::Color::sBoardPads);
  } else if (mPad.getSolderLayer() == Layer::topCopper()) {
    setZValue(BoardGraphicsScene::ZValue_FootprintPadsTop);
    mGraphicsItem->setLayer(Theme::Color::sBoardCopperTop);
  } else {
    setZValue(BoardGraphicsScene::ZValue_FootprintPadsBottom);
    mGraphicsItem->setLayer(Theme::Color::sBoardCopperBot);
  }
}

void BGI_FootprintPad::updateToolTip() noexcept {
  Q_ASSERT(mGraphicsItem);
  QString s;
  s += "<b>" % tr("Pad:") % " ";
  if (const PackagePad* pad = mPad.getLibPackagePad()) {
    s += *pad->getName();
  } else {
    s += "✖";
  }
  s += "</b><br>" % tr("Signal:") % " ";
  if (const ComponentSignalInstance* sig = mPad.getComponentSignalInstance()) {
    s += *sig->getCompSignal().getName();
  } else {
    s += "✖";
  }
  s += "<br>" % tr("Net:") % " ";
  if (const NetSignal* net = mPad.getCompSigInstNetSignal()) {
    s += *net->getName();
  } else {
    s += "✖";
  }
  mGraphicsItem->setToolTipText(s);
}

void BGI_FootprintPad::updateHightlighted(bool selected) noexcept {
  mGraphicsItem->setSelected(
      selected ||
      mHighlightedNetSignals->contains(mPad.getCompSigInstNetSignal()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
