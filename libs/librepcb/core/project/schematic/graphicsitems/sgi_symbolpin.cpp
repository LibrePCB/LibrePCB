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

#include "../../../library/sym/symbolpin.h"
#include "../../../utils/toolbox.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../items/si_symbol.h"
#include "../items/si_symbolpin.h"
#include "../schematiclayerprovider.h"

#include <librepcb/core/graphics/linegraphicsitem.h>
#include <librepcb/core/graphics/primitivecirclegraphicsitem.h>
#include <librepcb/core/graphics/primitivetextgraphicsitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_SymbolPin::SGI_SymbolPin(SI_SymbolPin& pin) noexcept
  : SGI_Base(),
    mPin(pin),
    mCircleGraphicsItem(new PrimitiveCircleGraphicsItem(this)),
    mLineGraphicsItem(new LineGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)) {
  setFlag(QGraphicsItem::ItemHasNoContents, false);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(Schematic::ZValue_Symbols);
  setToolTip(*mPin.getLibPin().getName());

  // Setup circle.
  mCircleGraphicsItem->setDiameter(UnsignedLength(1200000));
  mCircleGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mCircleGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

  // Setup line.
  mLineGraphicsItem->setLine(Point(0, 0),
                             Point(*mPin.getLibPin().getLength(), 0));
  mLineGraphicsItem->setLineWidth(UnsignedLength(158750));
  mLineGraphicsItem->setLayer(getLayer(GraphicsLayer::sSymbolOutlines));
  mLineGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mLineGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

  // Setup text.
  mTextGraphicsItem->setPosition(mPin.getLibPin().getNamePosition());
  mTextGraphicsItem->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
  mTextGraphicsItem->setHeight(SymbolPin::getNameHeight());
  mTextGraphicsItem->setLayer(getLayer(GraphicsLayer::sSymbolPinNames));
  mTextGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mTextGraphicsItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
  updateTextRotationAndAlignment();

  updateData();
}

SGI_SymbolPin::~SGI_SymbolPin() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SGI_SymbolPin::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void SGI_SymbolPin::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
  updateTextRotationAndAlignment();  // Auto-rotation may need to be updated.
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SGI_SymbolPin::updateData() noexcept {
  mTextGraphicsItem->setText(mPin.getDisplayText());

  bool isConnected = mPin.getCompSigInstNetSignal();
  const GraphicsLayer* lineLayer = nullptr;
  const GraphicsLayer* fillLayer = nullptr;
  if (mPin.isVisibleJunction()) {
    fillLayer = getLayer(GraphicsLayer::sSchematicNetLines);
  } else if ((!isConnected) && mPin.isRequired()) {
    lineLayer = getLayer(GraphicsLayer::sSymbolPinCirclesReq);
  } else if (!isConnected) {
    lineLayer = getLayer(GraphicsLayer::sSymbolPinCirclesOpt);
  }
  mCircleGraphicsItem->setLineLayer(lineLayer);
  mCircleGraphicsItem->setFillLayer(fillLayer);
}

void SGI_SymbolPin::updateSelection() noexcept {
  const bool selected = mPin.isSelected();
  const bool highlighted = mPin.getCompSigInstNetSignal() &&
      mPin.getCompSigInstNetSignal()->isHighlighted();
  mCircleGraphicsItem->setSelected(selected || highlighted);
  mLineGraphicsItem->setSelected(selected || highlighted);
  mTextGraphicsItem->setSelected(selected || highlighted);
  QGraphicsItem::setSelected(selected);  // Ignore highlighting here.
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QRectF SGI_SymbolPin::boundingRect() const noexcept {
  // It seems that the tooltip is not shown without this :-(
  return mCircleGraphicsItem->boundingRect();
}

QPainterPath SGI_SymbolPin::shape() const noexcept {
  QPainterPath p;
  p.addEllipse(mCircleGraphicsItem->boundingRect());
  return p;
}

void SGI_SymbolPin::paint(QPainter* painter,
                          const QStyleOptionGraphicsItem* option,
                          QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_SymbolPin::updateTextRotationAndAlignment() noexcept {
  Angle rotation(0);
  Alignment alignment(HAlign::left(), VAlign::center());
  if (Toolbox::isTextUpsideDown(mPin.getRotation(), false)) {
    rotation += Angle::deg180();
    alignment.mirror();
  }
  mTextGraphicsItem->setRotation(rotation);
  mTextGraphicsItem->setAlignment(alignment);
}

GraphicsLayer* SGI_SymbolPin::getLayer(const QString& name) const noexcept {
  return mPin.getSymbol().getProject().getLayers().getLayer(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
