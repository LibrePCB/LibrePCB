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

#include "graphicslayer.h"
#include "origincrossgraphicsitem.h"
#include "primitivepathgraphicsitem.h"

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

StrokeTextGraphicsItem::StrokeTextGraphicsItem(
    StrokeText& text, const IF_GraphicsLayerProvider& lp,
    const StrokeFont& font, QGraphicsItem* parent) noexcept
  : QGraphicsItemGroup(parent),
    mText(text),
    mLayerProvider(lp),
    mFont(font),
    mTextOverride(tl::nullopt),
    mPathGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mOriginCrossGraphicsItem(new OriginCrossGraphicsItem(this)),
    mOnEditedSlot(*this, &StrokeTextGraphicsItem::strokeTextEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  mPathGraphicsItem->setLineWidth(mText.getStrokeWidth());
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1000000));

  setPos(mText.getPosition().toPxQPointF());
  updateLayer(mText.getLayer());
  updateText();
  updateTransform();

  // register to the text to get attribute updates
  mText.onEdited.attach(mOnEditedSlot);
}

StrokeTextGraphicsItem::~StrokeTextGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void StrokeTextGraphicsItem::setTextOverride(
    const tl::optional<QString>& text) noexcept {
  if (text != mTextOverride) {
    mTextOverride = text;
    updateText();
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath StrokeTextGraphicsItem::shape() const noexcept {
  Q_ASSERT(mPathGraphicsItem && mOriginCrossGraphicsItem);
  return mPathGraphicsItem->shape() | mOriginCrossGraphicsItem->shape();
}

QVariant StrokeTextGraphicsItem::itemChange(GraphicsItemChange change,
                                            const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mPathGraphicsItem &&
      mOriginCrossGraphicsItem) {
    mPathGraphicsItem->setSelected(value.toBool());
    mOriginCrossGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void StrokeTextGraphicsItem::strokeTextEdited(
    const StrokeText& text, StrokeText::Event event) noexcept {
  switch (event) {
    case StrokeText::Event::LayerChanged:
      updateLayer(text.getLayer());
      break;
    case StrokeText::Event::TextChanged:
    case StrokeText::Event::HeightChanged:
    case StrokeText::Event::LetterSpacingChanged:
    case StrokeText::Event::LineSpacingChanged:
    case StrokeText::Event::AlignChanged:
    case StrokeText::Event::AutoRotateChanged:
      updateText();
      break;
    case StrokeText::Event::PositionChanged:
      setPos(text.getPosition().toPxQPointF());
      break;
    case StrokeText::Event::RotationChanged:
    case StrokeText::Event::MirroredChanged:
      updateTransform();
      updateText();  // Auto-rotation might have changed.
      break;
    case StrokeText::Event::StrokeWidthChanged:
      mPathGraphicsItem->setLineWidth(text.getStrokeWidth());
      updateText();  // Spacing might need to be re-calculated.
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "StrokeTextGraphicsItem::strokeTextEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void StrokeTextGraphicsItem::updateLayer(const Layer& layer) noexcept {
  const GraphicsLayer* obj = mLayerProvider.getLayer(layer);
  mPathGraphicsItem->setLineLayer(obj);
  mOriginCrossGraphicsItem->setLayer(obj);
}

void StrokeTextGraphicsItem::updateText() noexcept {
  const QString text = mTextOverride ? (*mTextOverride) : mText.getText();
  mPathGraphicsItem->setPath(
      Path::toQPainterPathPx(mText.generatePaths(mFont, text), false));
}

void StrokeTextGraphicsItem::updateTransform() noexcept {
  QTransform t;
  if (mText.getMirrored()) t.scale(qreal(-1), qreal(1));
  t.rotate(-mText.getRotation().toDeg());
  setTransform(t);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
