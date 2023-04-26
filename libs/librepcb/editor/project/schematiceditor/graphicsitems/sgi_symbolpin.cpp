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
#include "sgi_symbolpin.h"

#include "../../../graphics/linegraphicsitem.h"
#include "../../../graphics/primitivecirclegraphicsitem.h"
#include "../../../graphics/primitivetextgraphicsitem.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/utils/transform.h>
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

SGI_SymbolPin::SGI_SymbolPin(SI_SymbolPin& pin,
                             std::weak_ptr<SGI_Symbol> symbolItem,
                             const IF_GraphicsLayerProvider& lp,
                             std::shared_ptr<const QSet<const NetSignal*>>
                                 highlightedNetSignals) noexcept
  : QGraphicsItemGroup(),
    mPin(pin),
    mSymbolGraphicsItem(symbolItem),
    mLayerProvider(lp),
    mHighlightedNetSignals(highlightedNetSignals),
    mCircleGraphicsItem(new PrimitiveCircleGraphicsItem(this)),
    mLineGraphicsItem(new LineGraphicsItem(this)),
    mNameGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mNumbersGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mOnPinEditedSlot(*this, &SGI_SymbolPin::pinEdited),
    mOnSymbolEditedSlot(*this, &SGI_SymbolPin::symbolGraphicsItemEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_SymbolPins);
  setToolTip(*mPin.getLibPin().getName());

  // Setup circle.
  mCircleGraphicsItem->setDiameter(UnsignedLength(1200000));
  mCircleGraphicsItem->setShapeMode(
      PrimitiveCircleGraphicsItem::ShapeMode::FilledOutline);
  mCircleGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mCircleGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

  // Setup line.
  mLineGraphicsItem->setLine(Point(0, 0),
                             Point(*mPin.getLibPin().getLength(), 0));
  mLineGraphicsItem->setLineWidth(UnsignedLength(158750));
  mLineGraphicsItem->setLayer(
      mLayerProvider.getLayer(Theme::Color::sSchematicPinLines));
  mLineGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mLineGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

  // Setup name text.
  mNameGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mNameGraphicsItem->setHeight(mPin.getLibPin().getNameHeight());
  mNameGraphicsItem->setLayer(
      mLayerProvider.getLayer(Theme::Color::sSchematicPinNames));
  mNameGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mNameGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

  // Setup number text.
  mNumbersGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mNumbersGraphicsItem->setHeight(PositiveLength(1500000));
  mNumbersGraphicsItem->setLayer(
      mLayerProvider.getLayer(Theme::Color::sSchematicPinNumbers));
  mNumbersGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mNumbersGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

  updatePosition();
  updateRotation();
  updateJunction();
  updateName();
  updateNumbers();
  updateNumbersPosition();
  updateNumbersAlignment();
  updateToolTip();
  updateHighlightedState();

  mPin.onEdited.attach(mOnPinEditedSlot);
  if (auto ptr = mSymbolGraphicsItem.lock()) {
    ptr->onEdited.attach(mOnSymbolEditedSlot);
  }
}

SGI_SymbolPin::~SGI_SymbolPin() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SGI_SymbolPin::updateHighlightedState() noexcept {
  const bool highlight = isSelected() ||
      mHighlightedNetSignals->contains(mPin.getCompSigInstNetSignal());
  mCircleGraphicsItem->setSelected(highlight);
  mLineGraphicsItem->setSelected(highlight);
  mNameGraphicsItem->setSelected(highlight);
  mNumbersGraphicsItem->setSelected(highlight);
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath SGI_SymbolPin::shape() const noexcept {
  Q_ASSERT(mCircleGraphicsItem);
  return mCircleGraphicsItem->shape();
}

QVariant SGI_SymbolPin::itemChange(GraphicsItemChange change,
                                   const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mCircleGraphicsItem &&
      mLineGraphicsItem && mNameGraphicsItem && mNumbersGraphicsItem) {
    updateHighlightedState();
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_SymbolPin::pinEdited(const SI_SymbolPin& obj,
                              SI_SymbolPin::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_SymbolPin::Event::PositionChanged:
      updatePosition();
      break;
    case SI_SymbolPin::Event::RotationChanged:
      updateRotation();
      updateNumbersPosition();
      break;
    case SI_SymbolPin::Event::JunctionChanged:
      updateJunction();
      break;
    case SI_SymbolPin::Event::NameChanged:
      updateName();
      updateToolTip();
      break;
    case SI_SymbolPin::Event::NumbersChanged:
      updateNumbers();
      updateToolTip();
      break;
    case SI_SymbolPin::Event::NumbersPositionChanged:
      updateNumbersPosition();
      break;
    case SI_SymbolPin::Event::NumbersAlignmentChanged:
      updateNumbersAlignment();
      break;
    case SI_SymbolPin::Event::NetNameChanged:
      updateToolTip();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_SymbolPin::pinEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_SymbolPin::symbolGraphicsItemEdited(const SGI_Symbol& obj,
                                             SGI_Symbol::Event event) noexcept {
  if (event == SGI_Symbol::Event::SelectionChanged) {
    setSelected(obj.isSelected());
  }
}

void SGI_SymbolPin::updatePosition() noexcept {
  setPos(mPin.getPosition().toPxQPointF());
}

void SGI_SymbolPin::updateRotation() noexcept {
  Q_ASSERT(mLineGraphicsItem && mNameGraphicsItem && mNumbersGraphicsItem);

  mLineGraphicsItem->setRotation(mPin.getRotation());

  const Transform transform(mPin.getSymbol());
  const Point namePosition =
      transform.map(mPin.getLibPin().getNamePosition().rotated(
          mPin.getLibPin().getRotation())) -
      transform.getPosition();
  Angle nameRotation = transform.mapNonMirrorable(
      mPin.getLibPin().getRotation() + mPin.getLibPin().getNameRotation());
  Alignment nameAlignment = mPin.getLibPin().getNameAlignment();
  if (transform.getMirrored()) {
    nameAlignment.mirrorV();
  }
  mNameGraphicsItem->setPosition(namePosition);
  mNameGraphicsItem->setRotation(nameRotation);
  mNameGraphicsItem->setAlignment(nameAlignment);

  mNumbersGraphicsItem->setRotation(mPin.getRotation());
}

void SGI_SymbolPin::updateJunction() noexcept {
  Q_ASSERT(mCircleGraphicsItem);

  bool isConnected = mPin.getCompSigInstNetSignal();
  std::shared_ptr<GraphicsLayer> lineLayer = nullptr;
  std::shared_ptr<GraphicsLayer> fillLayer = nullptr;
  if (mPin.isVisibleJunction()) {
    fillLayer = mLayerProvider.getLayer(Theme::Color::sSchematicWires);
  } else if ((!isConnected) && mPin.isRequired()) {
    lineLayer = mLayerProvider.getLayer(Theme::Color::sSchematicRequiredPins);
  } else if (!isConnected) {
    lineLayer = mLayerProvider.getLayer(Theme::Color::sSchematicOptionalPins);
  }
  mCircleGraphicsItem->setLineLayer(lineLayer);
  mCircleGraphicsItem->setFillLayer(fillLayer);
}

void SGI_SymbolPin::updateName() noexcept {
  Q_ASSERT(mNameGraphicsItem);
  mNameGraphicsItem->setText(mPin.getName());
}

void SGI_SymbolPin::updateNumbers() noexcept {
  Q_ASSERT(mNumbersGraphicsItem);
  mNumbersGraphicsItem->setText(mPin.getNumbersTruncated());
}

void SGI_SymbolPin::updateNumbersPosition() noexcept {
  Q_ASSERT(mNumbersGraphicsItem);
  mNumbersGraphicsItem->setPosition(
      mPin.getNumbersPosition().rotated(mPin.getRotation()));
}

void SGI_SymbolPin::updateNumbersAlignment() noexcept {
  Q_ASSERT(mNumbersGraphicsItem);
  mNumbersGraphicsItem->setAlignment(mPin.getNumbersAlignment());
}

void SGI_SymbolPin::updateToolTip() noexcept {
  Q_ASSERT(mCircleGraphicsItem && mNameGraphicsItem && mNumbersGraphicsItem);
  QString s;
  s += "<b>" % tr("Signal:") % " ";
  if (const ComponentSignalInstance* sig = mPin.getComponentSignalInstance()) {
    s += *sig->getCompSignal().getName();
  } else {
    s += "✖";
  }
  s += "</b><br>" % tr("Net:") % " ";
  if (const NetSignal* net = mPin.getCompSigInstNetSignal()) {
    s += *net->getName();
  } else {
    s += "✖";
  }
  s += "<br>" % tr("Pin:") % " " % *mPin.getLibPin().getName();
  s += "<br>" % tr("Pad(s):") % " " % mPin.getNumbers().join(", ");
  mCircleGraphicsItem->setToolTip(s);
  mNameGraphicsItem->setToolTip(s);
  mNumbersGraphicsItem->setToolTip(s);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
