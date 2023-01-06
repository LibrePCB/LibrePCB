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
#include "boardairwiresbuilder.h"

#include "../../algorithm/airwiresbuilder.h"
#include "../../graphics/graphicslayer.h"
#include "../../library/pkg/footprintpad.h"
#include "../circuit/circuit.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "board.h"
#include "items/bi_footprintpad.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardAirWiresBuilder::BoardAirWiresBuilder(const Board& board,
                                           const NetSignal& netsignal) noexcept
  : mBoard(board), mNetSignal(netsignal) {
}

BoardAirWiresBuilder::~BoardAirWiresBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVector<QPair<Point, Point>> BoardAirWiresBuilder::buildAirWires() const {
  AirWiresBuilder builder;
  QHash<int, std::pair<Point, QString>> pointLayerMap;  // ID -> (point, layer)
  QHash<const BI_NetLineAnchor*, int> anchorMap;  // anchor -> ID

  // pads
  foreach (ComponentSignalInstance* cmpSig, mNetSignal.getComponentSignals()) {
    Q_ASSERT(cmpSig);
    foreach (BI_FootprintPad* pad, cmpSig->getRegisteredFootprintPads()) {
      if (&pad->getBoard() != &mBoard) continue;
      const Point& pos = pad->getPosition();
      int id = builder.addPoint(pos);
      pointLayerMap[id] = std::make_pair(pos,
                                         (pad->getLibPad().isTht())
                                             ? QString()  // on all layers
                                             : pad->getLayerName());
      anchorMap[pad] = id;
    }
  }

  // vias, netpoints, netlines
  foreach (const BI_NetSegment* netsegment, mNetSignal.getBoardNetSegments()) {
    Q_ASSERT(netsegment);
    if (&netsegment->getBoard() != &mBoard) continue;
    foreach (const BI_Via* via, netsegment->getVias()) {
      Q_ASSERT(via);
      const Point& pos = via->getPosition();
      int id = builder.addPoint(pos);
      pointLayerMap[id] = std::make_pair(pos, QString());  // on all layers
      anchorMap[via] = id;
    }
    foreach (const BI_NetPoint* netpoint, netsegment->getNetPoints()) {
      Q_ASSERT(netpoint);
      if (const GraphicsLayer* layer = netpoint->getLayerOfLines()) {
        Point pos = netpoint->getPosition();
        int id = builder.addPoint(pos);
        pointLayerMap[id] = std::make_pair(pos, layer->getName());
        anchorMap[netpoint] = id;
      }
    }
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      Q_ASSERT(netline);
      Q_ASSERT(anchorMap.contains(&netline->getStartPoint()));
      Q_ASSERT(anchorMap.contains(&netline->getEndPoint()));
      builder.addEdge(anchorMap[&netline->getStartPoint()],
                      anchorMap[&netline->getEndPoint()]);
    }
  }

  // determine connections made by planes
  foreach (const BI_Plane* plane, mNetSignal.getBoardPlanes()) {
    Q_ASSERT(plane);
    if (&plane->getBoard() != &mBoard) continue;
    foreach (const Path& fragment, plane->getFragments()) {
      int lastId = -1;
      QHashIterator<int, std::pair<Point, QString>> i(pointLayerMap);
      while (i.hasNext()) {
        i.next();
        const Point& pos = i.value().first;
        const QString& pointLayer = i.value().second;
        if (pointLayer.isNull() || (pointLayer == plane->getLayerName())) {
          if (fragment.toQPainterPathPx().contains(pos.toPxQPointF())) {
            if (lastId >= 0) {
              builder.addEdge(lastId, i.key());
            }
            lastId = i.key();
          }
        }
      }
    }
  }

  return builder.buildAirWires();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
