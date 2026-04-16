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
#include "sgi_netline.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/colorrole.h>

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

SGI_NetLine::SGI_NetLine(
    SI_NetLine& netline, const GraphicsLayerList& layers,
    std::shared_ptr<const SchematicGraphicsScene::Context> context) noexcept
  : QGraphicsItem(),
    mNetLine(netline),
    mContext(context),
    mLayer(layers.get(ColorRole::schematicWires())),
    mOnNetLineEditedSlot(*this, &SGI_NetLine::netLineEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_NetLines);

  updatePositions();

  mNetLine.onEdited.attach(mOnNetLineEditedSlot);
}

SGI_NetLine::~SGI_NetLine() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath SGI_NetLine::shape() const noexcept {
  return (mLayer && mLayer->isVisible()) ? mShape : QPainterPath();
}

void SGI_NetLine::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget) noexcept {
  Q_UNUSED(widget);

  if ((!mLayer) || (!mLayer->isVisible())) {
    return;
  }

  const NetSignal* net = &mNetLine.getNetSegment().getNetSignal();
  const bool selected = option->state.testFlag(QStyle::State_Selected);
  const GraphicsLayer::State state = mContext->getLayerState(selected, net);

  painter->setPen(QPen(mLayer->getColor(state), mNetLine.getWidth()->toPx(),
                       Qt::SolidLine, Qt::RoundCap));
  // See https://github.com/LibrePCB/LibrePCB/issues/1440
  mLineF.isNull() ? painter->drawPoint(mLineF.p1()) : painter->drawLine(mLineF);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_NetLine::netLineEdited(const SI_NetLine& obj,
                                SI_NetLine::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_NetLine::Event::PositionsChanged:
      updatePositions();
      break;
    case SI_NetLine::Event::NetSignalNameChanged:
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_NetLine::netLineEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_NetLine::updatePositions() noexcept {
  prepareGeometryChange();
  mLineF.setP1(mNetLine.getP1().getPosition().toPxQPointF());
  mLineF.setP2(mNetLine.getP2().getPosition().toPxQPointF());
  mBoundingRect = QRectF(mLineF.p1(), mLineF.p2()).normalized();
  mBoundingRect.adjust(
      -mNetLine.getWidth()->toPx() / 2, -mNetLine.getWidth()->toPx() / 2,
      mNetLine.getWidth()->toPx() / 2, mNetLine.getWidth()->toPx() / 2);
  mShape = QPainterPath();
  mShape.moveTo(mNetLine.getP1().getPosition().toPxQPointF());
  mShape.lineTo(mNetLine.getP2().getPosition().toPxQPointF());
  mShape = Toolbox::shapeFromPath(mShape, QPen(Qt::SolidPattern, 0), QBrush(),
                                  mNetLine.getWidth());
  update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
