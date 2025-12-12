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
#include "sgi_buslabel.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../../../graphics/linegraphicsitem.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/application.h>
#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/utils/overlinemarkupparser.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

QVector<QLineF> SGI_BusLabel::sOriginCrossLines;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_BusLabel::SGI_BusLabel(SI_BusLabel& label,
                           const GraphicsLayerList& layers) noexcept
  : QGraphicsItem(),
    mBusLabel(label),
    mOriginCrossLayer(layers.get(Theme::Color::sSchematicReferences)),
    mBusLabelLayer(layers.get(Theme::Color::sSchematicBusLabels)),
    mOnEditedSlot(*this, &SGI_BusLabel::busLabelEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_Buses);

  mStaticText.setTextFormat(Qt::PlainText);
  mStaticText.setPerformanceHint(QStaticText::AggressiveCaching);

  mFont = Application::getDefaultMonospaceFont();
  mFont.setPixelSize(4);

  if (sOriginCrossLines.isEmpty()) {
    qreal crossSizePx = Length(400000).toPx();
    sOriginCrossLines.append(QLineF(-crossSizePx, 0, crossSizePx, 0));
    sOriginCrossLines.append(QLineF(0, -crossSizePx, 0, crossSizePx));
  }

  // create anchor graphics item
  mAnchorGraphicsItem.reset(new LineGraphicsItem());
  mAnchorGraphicsItem->setZValue(SchematicGraphicsScene::ZValue_Buses);
  mAnchorGraphicsItem->setLayer(layers.get(Theme::Color::sSchematicReferences));
  mAnchorGraphicsItem->setSelected(isSelected());

  updatePosition();
  updateRotation();
  updateText();
  updateAnchor();

  mBusLabel.onEdited.attach(mOnEditedSlot);
}

SGI_BusLabel::~SGI_BusLabel() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SGI_BusLabel::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) noexcept {
  Q_UNUSED(widget);

  // If the net label layer is disabled, do not draw anything.
  if ((!mBusLabelLayer) || (!mBusLabelLayer->isVisible())) {
    return;
  }

  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());
  const bool highlight = option->state.testFlag(QStyle::State_Selected);

  if (mOriginCrossLayer && mOriginCrossLayer->isVisible() && (lod > 2)) {
    // draw origin cross
    painter->setPen(QPen(mOriginCrossLayer->getColor(highlight), 0));
    painter->drawLines(sOriginCrossLines);
  }

  if (lod > 1) {
    // draw text
    painter->setPen(QPen(mBusLabelLayer->getColor(highlight), 0));
    painter->setFont(mFont);
    painter->save();
    if (mRotate180) {
      painter->rotate(180);
    }
    painter->drawStaticText(mTextOrigin, mStaticText);
    painter->setPen(QPen(mBusLabelLayer->getColor(highlight), qreal(4) / 15));
    painter->drawLines(mOverlines);
    painter->restore();
  } else {
    // draw filled rect
    painter->setPen(Qt::NoPen);
    painter->setBrush(
        QBrush(mBusLabelLayer->getColor(highlight), Qt::Dense5Pattern));
    painter->drawRect(mBoundingRect);
  }
}

QVariant SGI_BusLabel::itemChange(GraphicsItemChange change,
                                  const QVariant& value) noexcept {
  if ((change == ItemSceneHasChanged) && mAnchorGraphicsItem) {
    if (QGraphicsScene* s = mAnchorGraphicsItem->scene()) {
      s->removeItem(mAnchorGraphicsItem.data());
    }
    if (QGraphicsScene* s = scene()) {
      s->addItem(mAnchorGraphicsItem.data());
    }
  } else if ((change == ItemSelectedHasChanged) && mAnchorGraphicsItem) {
    mAnchorGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_BusLabel::busLabelEdited(const SI_BusLabel& obj,
                                  SI_BusLabel::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_BusLabel::Event::PositionChanged:
      updatePosition();
      updateAnchor();
      break;
    case SI_BusLabel::Event::RotationChanged:
      updateRotation();
      updateText();
      break;
    case SI_BusLabel::Event::MirroredChanged:
    case SI_BusLabel::Event::BusNameChanged:
      updateText();
      break;
    case SI_BusLabel::Event::AnchorPositionChanged:
      updateAnchor();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_BusLabel::netLabelEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_BusLabel::updatePosition() noexcept {
  setPos(mBusLabel.getPosition().toPxQPointF());
}

void SGI_BusLabel::updateRotation() noexcept {
  setRotation(-mBusLabel.getRotation().toDeg());
}

void SGI_BusLabel::updateText() noexcept {
  prepareGeometryChange();

  mRotate180 = Toolbox::isTextUpsideDown(mBusLabel.getRotation());

  const Alignment align(
      mBusLabel.getMirrored() ? HAlign::right() : HAlign::left(),
      VAlign::bottom());
  const int flags =
      mRotate180 ? align.mirrored().toQtAlign() : align.toQtAlign();

  QString displayText;
  const QFontMetricsF fm(mFont);
  OverlineMarkupParser::process(*mBusLabel.getBusSegment().getBus().getName(),
                                fm, flags, displayText, mOverlines,
                                mBoundingRect);

  mStaticText.setText(displayText);
  mStaticText.prepare(QTransform(), mFont);
  if (mBusLabel.getMirrored() ^ mRotate180) {
    mTextOrigin.setX(-mStaticText.size().width());
  } else {
    mTextOrigin.setX(0);
  }
  mTextOrigin.setY(mRotate180 ? 0 : -mStaticText.size().height());
  mStaticText.prepare(QTransform()
                          .rotate(mRotate180 ? 180 : 0)
                          .translate(mTextOrigin.x(), mTextOrigin.y()),
                      mFont);

  QRectF rect =
      QRectF(0, 0, mStaticText.size().width(), -mStaticText.size().height())
          .normalized();

  if (mBusLabel.getMirrored()) rect.moveLeft(-mStaticText.size().width());

  qreal len = sOriginCrossLines[0].length();
  mBoundingRect =
      rect.united(QRectF(-len / 2, -len / 2, len, len)).normalized();

  update();
}

void SGI_BusLabel::updateAnchor() noexcept {
  mAnchorGraphicsItem->setLine(mBusLabel.getPosition(),
                               mBusLabel.getAnchorPosition());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
