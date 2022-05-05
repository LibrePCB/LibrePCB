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
#include "stroketextgraphicsitem.h"

#include "../attribute/attributesubstitutor.h"
#include "../graphics/graphicslayer.h"
#include "origincrossgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StrokeTextGraphicsItem::StrokeTextGraphicsItem(
    StrokeText& text, const IF_GraphicsLayerProvider& lp,
    const StrokeFont& font, QGraphicsItem* parent) noexcept
  : PrimitivePathGraphicsItem(parent),
    mText(text),
    mLayerProvider(lp),
    mFont(font),
    mAttributeProvider(nullptr),
    mSubstitutedText(),
    mOnEditedSlot(*this, &StrokeTextGraphicsItem::strokeTextEdited) {
  // add origin cross
  mOriginCrossGraphicsItem.reset(new OriginCrossGraphicsItem(this));
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));

  // set text properties
  setPosition(mText.getPosition());
  setLineWidth(mText.getStrokeWidth());
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);
  updateLayer(mText.getLayerName());
  updateText();
  updateTransform();

  // register to the text to get attribute updates
  mText.onEdited.attach(mOnEditedSlot);
}

StrokeTextGraphicsItem::~StrokeTextGraphicsItem() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void StrokeTextGraphicsItem::setAttributeProvider(
    const AttributeProvider* provider) noexcept {
  if (provider != mAttributeProvider) {
    mAttributeProvider = provider;
    updateText();
  }
}

void StrokeTextGraphicsItem::updateText() noexcept {
  QString text = mText.getText();
  if (mAttributeProvider) {
    text =
        AttributeSubstitutor::substitute(mText.getText(), mAttributeProvider);
  }
  if (text != mSubstitutedText) {
    mSubstitutedText = text;
    updatePaths();
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath StrokeTextGraphicsItem::shape() const noexcept {
  return PrimitivePathGraphicsItem::shape() + mOriginCrossGraphicsItem->shape();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void StrokeTextGraphicsItem::strokeTextEdited(
    const StrokeText& text, StrokeText::Event event) noexcept {
  switch (event) {
    case StrokeText::Event::LayerNameChanged:
      updateLayer(text.getLayerName());
      break;
    case StrokeText::Event::TextChanged:
      updateText();
      break;
    case StrokeText::Event::HeightChanged:
    case StrokeText::Event::LetterSpacingChanged:
    case StrokeText::Event::LineSpacingChanged:
    case StrokeText::Event::AlignChanged:
    case StrokeText::Event::AutoRotateChanged:
      updatePaths();
      break;
    case StrokeText::Event::PositionChanged:
      setPosition(text.getPosition());
      break;
    case StrokeText::Event::RotationChanged:
      updatePaths();  // Auto-rotation might have changed.
      updateTransform();
      break;
    case StrokeText::Event::StrokeWidthChanged:
      setLineWidth(text.getStrokeWidth());
      updatePaths();  // Spacing might need to be re-calculated.
      break;
    case StrokeText::Event::MirroredChanged:
      updatePaths();
      updateTransform();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "StrokeTextGraphicsItem::strokeTextEdited()";
      break;
  }
}

void StrokeTextGraphicsItem::updateLayer(
    const GraphicsLayerName& layerName) noexcept {
  const GraphicsLayer* layer = mLayerProvider.getLayer(*layerName);
  setLineLayer(layer);
  mOriginCrossGraphicsItem->setLayer(layer);
}

void StrokeTextGraphicsItem::updatePaths() noexcept {
  setPath(Path::toQPainterPathPx(mText.generatePaths(mFont, mSubstitutedText),
                                 false));
}

void StrokeTextGraphicsItem::updateTransform() noexcept {
  QTransform t;
  if (mText.getMirrored()) t.scale(qreal(-1), qreal(1));
  t.rotate(-mText.getRotation().toDeg());
  setTransform(t);
}

QVariant StrokeTextGraphicsItem::itemChange(GraphicsItemChange change,
                                            const QVariant& value) noexcept {
  if (change == ItemSelectedChange && mOriginCrossGraphicsItem) {
    mOriginCrossGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
