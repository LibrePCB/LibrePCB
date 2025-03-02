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

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/linegraphicsitem.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/application.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
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

QVector<QLineF> SGI_NetLabel::sOriginCrossLines;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_NetLabel::SGI_NetLabel(SI_NetLabel& netlabel,
                           const IF_GraphicsLayerProvider& lp,
                           std::shared_ptr<const QSet<const NetSignal*>>
                               highlightedNetSignals) noexcept
  : QGraphicsItem(),
    mNetLabel(netlabel),
    mHighlightedNetSignals(highlightedNetSignals),
    mOriginCrossLayer(lp.getLayer(Theme::Color::sSchematicReferences)),
    mNetLabelLayer(lp.getLayer(Theme::Color::sSchematicNetLabels)),
    mOnEditedSlot(*this, &SGI_NetLabel::netLabelEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_NetLabels);

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
  mAnchorGraphicsItem->setZValue(SchematicGraphicsScene::ZValue_NetLabels);
  mAnchorGraphicsItem->setLayer(
      lp.getLayer(Theme::Color::sSchematicNetLabelAnchors));

  updatePosition();
  updateRotation();
  updateText();
  updateAnchor();

  mNetLabel.onEdited.attach(mOnEditedSlot);
}

SGI_NetLabel::~SGI_NetLabel() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SGI_NetLabel::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) noexcept {
  Q_UNUSED(widget);

  // If the net label layer is disabled, do not draw anything.
  if ((!mNetLabelLayer) || (!mNetLabelLayer->isVisible())) {
    return;
  }

  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());
  const bool highlight = option->state.testFlag(QStyle::State_Selected) ||
      mHighlightedNetSignals->contains(&mNetLabel.getNetSignalOfNetSegment());

  if (mOriginCrossLayer && mOriginCrossLayer->isVisible() && (lod > 2)) {
    // draw origin cross
    painter->setPen(QPen(mOriginCrossLayer->getColor(highlight), 0));
    painter->drawLines(sOriginCrossLines);
  }

  if (lod > 1) {
    // draw text
    painter->setPen(QPen(mNetLabelLayer->getColor(highlight), 0));
    painter->setFont(mFont);
    painter->save();
    if (mRotate180) {
      painter->rotate(180);
    }
    painter->drawStaticText(mTextOrigin, mStaticText);
    painter->setPen(QPen(mNetLabelLayer->getColor(highlight), qreal(4) / 15));
    painter->drawLines(mOverlines);
    painter->restore();
  } else {
    // draw filled rect
    painter->setPen(Qt::NoPen);
    painter->setBrush(
        QBrush(mNetLabelLayer->getColor(highlight), Qt::Dense5Pattern));
    painter->drawRect(mBoundingRect);
  }
}

QVariant SGI_NetLabel::itemChange(GraphicsItemChange change,
                                  const QVariant& value) noexcept {
  if ((change == ItemSceneHasChanged) && mAnchorGraphicsItem) {
    if (QGraphicsScene* s = mAnchorGraphicsItem->scene()) {
      s->removeItem(mAnchorGraphicsItem.data());
    }
    if (QGraphicsScene* s = scene()) {
      s->addItem(mAnchorGraphicsItem.data());
    }
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_NetLabel::netLabelEdited(const SI_NetLabel& obj,
                                  SI_NetLabel::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_NetLabel::Event::PositionChanged:
      updatePosition();
      updateAnchor();
      break;
    case SI_NetLabel::Event::RotationChanged:
      updateRotation();
      updateText();
      break;
    case SI_NetLabel::Event::MirroredChanged:
    case SI_NetLabel::Event::NetNameChanged:
      updateText();
      break;
    case SI_NetLabel::Event::AnchorPositionChanged:
      updateAnchor();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_NetLabel::netLabelEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_NetLabel::updatePosition() noexcept {
  setPos(mNetLabel.getPosition().toPxQPointF());
}

void SGI_NetLabel::updateRotation() noexcept {
  setRotation(-mNetLabel.getRotation().toDeg());
}

void SGI_NetLabel::updateText() noexcept {
  prepareGeometryChange();

  mRotate180 = Toolbox::isTextUpsideDown(mNetLabel.getRotation());

  const Alignment align(
      mNetLabel.getMirrored() ? HAlign::right() : HAlign::left(),
      VAlign::bottom());
  const int flags =
      mRotate180 ? align.mirrored().toQtAlign() : align.toQtAlign();

  QString displayText;
  const QFontMetricsF fm(mFont);
  OverlineMarkupParser::process(*mNetLabel.getNetSignalOfNetSegment().getName(),
                                fm, flags, displayText, mOverlines,
                                mBoundingRect);

  mStaticText.setText(displayText);
  mStaticText.prepare(QTransform(), mFont);
  if (mNetLabel.getMirrored() ^ mRotate180) {
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

  if (mNetLabel.getMirrored()) rect.moveLeft(-mStaticText.size().width());

  qreal len = sOriginCrossLines[0].length();
  mBoundingRect =
      rect.united(QRectF(-len / 2, -len / 2, len, len)).normalized();

  update();
}

void SGI_NetLabel::updateAnchor() noexcept {
  mAnchorGraphicsItem->setLine(mNetLabel.getPosition(),
                               mNetLabel.getAnchorPosition());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
