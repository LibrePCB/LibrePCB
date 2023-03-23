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

QVector<std::pair<const BI_NetLineAnchor*, const BI_NetLineAnchor*>>
    BoardAirWiresBuilder::buildAirWires() const {
  AirWiresBuilder builder;
  QHash<int, std::pair<Point, const Layer*>> pointLayerMap;  // ID -> (p, layer)
  QHash<const BI_NetLineAnchor*, int> anchorMap;  // anchor -> ID

  // pads
  foreach (ComponentSignalInstance* cmpSig, mNetSignal.getComponentSignals()) {
    Q_ASSERT(cmpSig);
    foreach (BI_FootprintPad* pad, cmpSig->getRegisteredFootprintPads()) {
      if (&pad->getBoard() != &mBoard) continue;
      const Point& pos = pad->getPosition();
      int id = builder.addPoint(pos);
      pointLayerMap[id] =
          std::make_pair(pos,
                         (pad->getLibPad().isTht()) ? nullptr  // on all layers
                                                    : &pad->getSmtLayer());
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
      pointLayerMap[id] = std::make_pair(pos, nullptr);  // on all layers
      anchorMap[via] = id;
    }
    foreach (const BI_NetPoint* netpoint, netsegment->getNetPoints()) {
      Q_ASSERT(netpoint);
      if (const Layer* layer = netpoint->getLayerOfTraces()) {
        Point pos = netpoint->getPosition();
        int id = builder.addPoint(pos);
        pointLayerMap[id] = std::make_pair(pos, layer);
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
      QHashIterator<int, std::pair<Point, const Layer*>> i(pointLayerMap);
      while (i.hasNext()) {
        i.next();
        const Point& pos = i.value().first;
        const Layer* pointLayer = i.value().second;
        if ((!pointLayer) || (pointLayer == &plane->getLayer())) {
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

  // Calculate the airwires and convert them back to the result type.
  const AirWiresBuilder::AirWires airWireIds = builder.buildAirWires();
  QVector<std::pair<const BI_NetLineAnchor*, const BI_NetLineAnchor*>> result;
  result.reserve(airWireIds.size());
  foreach (const AirWiresBuilder::AirWire& airWire, airWireIds) {
    const BI_NetLineAnchor* p1 = anchorMap.key(airWire.first, nullptr);
    const BI_NetLineAnchor* p2 = anchorMap.key(airWire.second, nullptr);
    if ((!p1) || (!p2)) {
      throw LogicError(__FILE__, __LINE__, "Unknown air wire IDs received.");
    }
    result.append(std::make_pair(p1, p2));
  }

  return result;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
