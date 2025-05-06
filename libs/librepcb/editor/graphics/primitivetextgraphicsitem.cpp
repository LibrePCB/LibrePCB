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
#include "primitivetextgraphicsitem.h"

#include <librepcb/core/application.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/utils/overlinemarkupparser.h>
#include <librepcb/core/utils/toolbox.h>

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

PrimitiveTextGraphicsItem::PrimitiveTextGraphicsItem(
    QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mLayer(nullptr),
    mText(),
    mDisplayText(),
    mParseOverlines(false),
    mOverlines(),
    mHeight(1),
    mAlignment(HAlign::left(), VAlign::bottom()),
    mRotate180(false),
    mFont(Application::getDefaultSansSerifFont()),
    mTextFlags(0),
    mShapeEnabled(true),
    mOnLayerEditedSlot(*this, &PrimitiveTextGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  updateBoundingRectAndShape();
  setVisible(false);
}

PrimitiveTextGraphicsItem::~PrimitiveTextGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PrimitiveTextGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void PrimitiveTextGraphicsItem::setRotation(const Angle& rot) noexcept {
  const bool rotate180 = Toolbox::isTextUpsideDown(rot);
  if (rotate180 != mRotate180) {
    mRotate180 = rotate180;
    updateBoundingRectAndShape();
  }
  if (rotate180) {
    QGraphicsItem::setRotation(180 - rot.toDeg());
  } else {
    QGraphicsItem::setRotation(-rot.toDeg());
  }
}

void PrimitiveTextGraphicsItem::setText(const QString& text,
                                        bool parseOverlines) noexcept {
  if ((text != mText) || (parseOverlines != mParseOverlines)) {
    mText = text;
    mParseOverlines = parseOverlines;
    updateBoundingRectAndShape();
  }
}

void PrimitiveTextGraphicsItem::setHeight(
    const PositiveLength& height) noexcept {
  mHeight = height;
  mPen.setWidthF(OverlineMarkupParser::getLineWidth(height->toPx()));
  mPenHighlighted.setWidthF(OverlineMarkupParser::getLineWidth(height->toPx()));
  updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setAlignment(const Alignment& align) noexcept {
  mAlignment = align;
  updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setFont(Font font) noexcept {
  switch (font) {
    case Font::SansSerif:
      mFont = Application::getDefaultSansSerifFont();
      break;
    case Font::Monospace:
      mFont = Application::getDefaultMonospaceFont();
      break;
    default: {
      qCritical()
          << "Unhandled switch-case in PrimitiveTextGraphicsItem::setFont():"
          << static_cast<int>(font);
      break;
    }
  }
  updateBoundingRectAndShape();
}

void PrimitiveTextGraphicsItem::setLayer(
    const std::shared_ptr<const GraphicsLayer>& layer) noexcept {
  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = layer;
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
    mPen.setColor(mLayer->getColor(false));
    mPenHighlighted.setColor(mLayer->getColor(true));
    setVisible(mLayer->isVisible());
    update();
  } else {
    setVisible(false);
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath PrimitiveTextGraphicsItem::shape() const noexcept {
  return (mShapeEnabled && mLayer && mLayer->isVisible()) ? mShape
                                                          : QPainterPath();
}

void PrimitiveTextGraphicsItem::paint(QPainter* painter,
                                      const QStyleOptionGraphicsItem* option,
                                      QWidget* widget) noexcept {
  Q_UNUSED(widget);
  painter->setFont(mFont);
  if (option->state.testFlag(QStyle::State_Selected)) {
    painter->setPen(mPenHighlighted);
  } else {
    painter->setPen(mPen);
  }
  painter->drawText(QRectF(), mTextFlags, mDisplayText);
  painter->drawLines(mOverlines);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PrimitiveTextGraphicsItem::layerEdited(
    const GraphicsLayer& layer, GraphicsLayer::Event event) noexcept {
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      mPen.setColor(layer.getColor(false));
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      mPenHighlighted.setColor(layer.getColor(true));
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      setVisible(layer.isVisible() && layer.isEnabled());
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PrimitiveTextGraphicsItem::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PrimitiveTextGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  mTextFlags = Qt::TextDontClip;
  if (mRotate180) {
    mTextFlags |= mAlignment.mirrored().toQtAlign();
  } else {
    mTextFlags |= mAlignment.toQtAlign();
  }
  mFont.setPixelSize(qCeil(mHeight->toPx()));

  const QFontMetricsF fm(mFont);
  if (mParseOverlines) {
    OverlineMarkupParser::process(mText, fm, mTextFlags, mDisplayText,
                                  mOverlines, mBoundingRect);
  } else {
    mDisplayText = mText;
    mOverlines.clear();
    mBoundingRect = fm.boundingRect(QRectF(), mTextFlags, mText);
  }

  mShape = QPainterPath();
  mShape.addRect(mBoundingRect);
  setScale(mHeight->toPx() / fm.height());
  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
