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

#include "../../../graphics/circlegraphicsitem.h"
#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/origincrossgraphicsitem.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/utils/toolbox.h>
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

SGI_Symbol::SGI_Symbol(SI_Symbol& symbol,
                       const IF_GraphicsLayerProvider& lp) noexcept
  : QGraphicsItemGroup(),
    onEdited(*this),
    mSymbol(symbol),
    mOnEditedSlot(*this, &SGI_Symbol::symbolEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_Symbols);

  mOriginCrossGraphicsItem = std::make_shared<OriginCrossGraphicsItem>(this);
  mOriginCrossGraphicsItem->setSize(UnsignedLength(1400000));
  mOriginCrossGraphicsItem->setLayer(
      lp.getLayer(Theme::Color::sSchematicReferences));
  mShape.addRect(mOriginCrossGraphicsItem->boundingRect());

  // Draw grab areas first to make them appearing behind every other graphics
  // item. Otherwise they might completely cover (hide) other items.
  for (bool grabArea : {true, false}) {
    for (const auto& obj : mSymbol.getLibSymbol().getCircles()) {
      if (obj.isGrabArea() != grabArea) continue;
      auto i = std::make_shared<CircleGraphicsItem>(const_cast<Circle&>(obj),
                                                    lp, this);
      i->setFlag(QGraphicsItem::ItemIsSelectable, true);
      i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
      if (obj.isGrabArea()) {
        const qreal r = (obj.getDiameter() + obj.getLineWidth())->toPx() / 2;
        QPainterPath path;
        path.addEllipse(obj.getCenter().toPxQPointF(), r, r);
        mShape |= path;
      }
      mCircleGraphicsItems.append(i);
    }

    for (const auto& obj : mSymbol.getLibSymbol().getPolygons()) {
      if (obj.isGrabArea() != grabArea) continue;
      auto i = std::make_shared<PolygonGraphicsItem>(const_cast<Polygon&>(obj),
                                                     lp, this);
      i->setFlag(QGraphicsItem::ItemIsSelectable, true);
      i->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
      if (obj.isGrabArea()) {
        mShape |= Toolbox::shapeFromPath(obj.getPath().toQPainterPathPx(),
                                         Qt::SolidLine, Qt::SolidPattern,
                                         obj.getLineWidth());
      }
      mPolygonGraphicsItems.append(i);
    }
  }

  updatePosition();
  updateRotationAndMirrored();
  mSymbol.onEdited.attach(mOnEditedSlot);
}

SGI_Symbol::~SGI_Symbol() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_Symbol::symbolEdited(const SI_Symbol& obj,
                              SI_Symbol::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_Symbol::Event::PositionChanged:
      updatePosition();
      break;
    case SI_Symbol::Event::RotationChanged:
    case SI_Symbol::Event::MirroredChanged:
      updateRotationAndMirrored();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_Symbol::symbolEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_Symbol::updatePosition() noexcept {
  setPos(mSymbol.getPosition().toPxQPointF());
  onEdited.notify(Event::PositionChanged);
}

void SGI_Symbol::updateRotationAndMirrored() noexcept {
  QTransform t;
  t.rotate(-mSymbol.getRotation().toDeg());
  if (mSymbol.getMirrored()) t.scale(qreal(-1), qreal(1));
  setTransform(t);
}

QVariant SGI_Symbol::itemChange(GraphicsItemChange change,
                                const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mOriginCrossGraphicsItem) {
    mOriginCrossGraphicsItem->setSelected(value.toBool());
    foreach (const auto& i, mCircleGraphicsItems) {
      i->setSelected(value.toBool());
    }
    foreach (const auto& i, mPolygonGraphicsItems) {
      i->setSelected(value.toBool());
    }
    onEdited.notify(Event::SelectionChanged);
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
