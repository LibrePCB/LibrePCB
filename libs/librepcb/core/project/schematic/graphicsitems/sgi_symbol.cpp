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
#include "sgi_symbol.h"

#include "../../../attribute/attributesubstitutor.h"
#include "../../../library/sym/symbol.h"
#include "../../../utils/toolbox.h"
#include "../../../utils/transform.h"
#include "../../project.h"
#include "../items/si_symbol.h"
#include "../schematiclayerprovider.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/origincrossgraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
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

SGI_Symbol::SGI_Symbol(SI_Symbol& symbol) noexcept
  : SGI_Base(), mSymbol(symbol) {
  setFlag(QGraphicsItem::ItemHasNoContents, false);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(Schematic::ZValue_Symbols);

  mOriginCrossGraphicsItem = std::make_shared<OriginCrossGraphicsItem>(this);
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1400000));
  mOriginCrossGraphicsItem->setLayer(mSymbol.getProject().getLayers().getLayer(
      GraphicsLayer::sSchematicReferences));
  mShape.addRect(mOriginCrossGraphicsItem->boundingRect());

  for (auto& obj : mSymbol.getLibSymbol().getCircles().values()) {
    Q_ASSERT(obj);
    auto i = std::make_shared<CircleGraphicsItem>(
        *obj, mSymbol.getProject().getLayers(), this);
    i->setFlag(QGraphicsItem::ItemIsSelectable, true);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    if (obj->isGrabArea()) {
      const qreal r = (obj->getDiameter() + obj->getLineWidth())->toPx() / 2;
      QPainterPath path;
      path.addEllipse(obj->getCenter().toPxQPointF(), r, r);
      mShape |= path;
    }
    mCircleGraphicsItems.append(i);
  }

  for (auto& obj : mSymbol.getLibSymbol().getPolygons().values()) {
    Q_ASSERT(obj);
    auto i = std::make_shared<PolygonGraphicsItem>(
        *obj, mSymbol.getProject().getLayers(), this);
    i->setFlag(QGraphicsItem::ItemIsSelectable, true);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    if (obj->isGrabArea()) {
      mShape |= Toolbox::shapeFromPath(obj->getPath().toQPainterPathPx(),
                                       Qt::SolidLine, Qt::SolidPattern,
                                       obj->getLineWidth());
    }
    mPolygonGraphicsItems.append(i);
  }

  for (auto& obj : mSymbol.getLibSymbol().getTexts().values()) {
    Q_ASSERT(obj);
    auto i = std::make_shared<PrimitiveTextGraphicsItem>(this);
    i->setPosition(obj->getPosition());
    i->setHeight(obj->getHeight());
    i->setLayer(
        mSymbol.getProject().getLayers().getLayer(*obj->getLayerName()));
    i->setFont(PrimitiveTextGraphicsItem::Font::SansSerif);
    i->setFlag(QGraphicsItem::ItemIsSelectable, true);
    i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    mTextGraphicsItems.append(i);
  }

  updateRotationAndMirror();
  updateAllTexts();

  mBoundingRect = childrenBoundingRect();
}

SGI_Symbol::~SGI_Symbol() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SGI_Symbol::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void SGI_Symbol::updateRotationAndMirror() noexcept {
  QTransform t;
  if (mSymbol.getMirrored()) t.scale(qreal(-1), qreal(1));
  const QTransform revertMirrorTransform = t;
  t.rotate(-mSymbol.getRotation().toDeg());
  setTransform(t);

  for (int i = 0; i < std::min(mSymbol.getLibSymbol().getTexts().count(),
                               mTextGraphicsItems.count());
       ++i) {
    const auto text = mSymbol.getLibSymbol().getTexts().at(i);
    Q_ASSERT(text);
    auto item = mTextGraphicsItems.at(i);
    Q_ASSERT(item);
    item->setTransform(revertMirrorTransform);
    Transform transform(mSymbol);
    Angle rotation = text->getRotation();
    Alignment alignment = text->getAlign();
    if (mSymbol.getMirrored()) {
      rotation += Angle::deg180();
      alignment.mirrorV();
    }
    if (Toolbox::isTextUpsideDown(transform.map(text->getRotation()), false)) {
      rotation += Angle::deg180();
      alignment.mirror();
    }
    item->setRotation(rotation);
    item->setAlignment(alignment);
  }
}

void SGI_Symbol::setSelected(bool selected) noexcept {
  mOriginCrossGraphicsItem->setSelected(selected);
  foreach (const auto& i, mCircleGraphicsItems) { i->setSelected(selected); }
  foreach (const auto& i, mPolygonGraphicsItems) { i->setSelected(selected); }
  foreach (const auto& i, mTextGraphicsItems) { i->setSelected(selected); }
  QGraphicsItem::setSelected(selected);
}

void SGI_Symbol::updateAllTexts() noexcept {
  for (int i = 0; i < std::min(mSymbol.getLibSymbol().getTexts().count(),
                               mTextGraphicsItems.count());
       ++i) {
    const auto text = mSymbol.getLibSymbol().getTexts().at(i);
    Q_ASSERT(text);
    auto item = mTextGraphicsItems.at(i);
    Q_ASSERT(item);
    item->setText(AttributeSubstitutor::substitute(text->getText(), &mSymbol));
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SGI_Symbol::paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option,
                       QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
