/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <unordered_map>
#include "boardairwiresbuilder.h"
#include "board.h"
#include "items/bi_netsegment.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include "items/bi_footprintpad.h"
#include "items/bi_via.h"
#include "items/bi_plane.h"
#include "../project.h"
#include "../circuit/circuit.h"
#include "../circuit/netsignal.h"
#include "../circuit/componentsignalinstance.h"
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <delaunay-triangulation/delaunay.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

// adapted from horizon/kicad
static QVector<QPair<Point, Point>> kruskalMst(
        std::vector<delaunay::Edge<qreal>>& aEdges,
        std::vector<delaunay::Vector2<qreal>>& aNodes) noexcept
{
    unsigned int nodeNumber = aNodes.size();
    unsigned int mstExpectedSize = nodeNumber - 1;
    unsigned int mstSize = 0;
    bool ratsnestLines = false;

    // printf("mst nodes : %d edges : %d\n", aNodes.size(), aEdges.size () );
    // The output
    QVector<QPair<Point, Point>> mst;

    // Set tags for marking cycles
    std::unordered_map<int, int> tags;
    unsigned int tag = 0;

    for (auto &node : aNodes) {
        node.tag = tag;
        tags[node.id] = tag++;
    }

    // Lists of nodes connected together (subtrees) to detect cycles in the
    // graph
    std::vector<std::list<int>> cycles(nodeNumber);

    for (unsigned int i = 0; i < nodeNumber; ++i)
        cycles[i].push_back(i);

    // Kruskal algorithm requires edges to be sorted by their weight
    std::sort(aEdges.begin(), aEdges.end(),
        [](const delaunay::Edge<qreal> &a, const delaunay::Edge<qreal> &b) {
            return a.weight > b.weight;
        });

    while (mstSize < mstExpectedSize && !aEdges.empty()) {
        auto &dt = aEdges.back();

        int srcTag = tags[dt.p1.id];
        int trgTag = tags[dt.p2.id];
        // printf("mstSize %d %d %f %d<>%d\n", mstSize, mstExpectedSize,
        // dt.weight, srcTag, trgTag);

        // Check if by adding this edge we are going to join two different
        // forests
        if (srcTag != trgTag) {
            // Because edges are sorted by their weight, first we always process
            // connected
            // items (weight == 0). Once we stumble upon an edge with non-zero
            // weight,
            // it means that the rest of the lines are ratsnest.
            if (!ratsnestLines && dt.weight >= 0)
                ratsnestLines = true;

            // Update tags
            if (ratsnestLines) {
                for (auto it = cycles[trgTag].begin(); it != cycles[trgTag].end(); ++it) {
                    tags[aNodes[*it].id] = srcTag;
                }

                // Do a copy of edge, but make it RN_EDGE_MST. In contrary to
                // RN_EDGE,
                // RN_EDGE_MST saves both source and target node and does not
                // require any other
                // edges to exist for getting source/target nodes
                // CN_EDGE newEdge ( dt.GetSourceNode(), dt.GetTargetNode(),
                // dt.GetWeight() );

                // assert( newEdge.GetSourceNode()->GetTag() !=
                // newEdge.GetTargetNode()->GetTag() );
                // assert(dt.p1.tag != dt.p2.tag);
                mst.append(qMakePair(Point(dt.p1.x, dt.p1.y), Point(dt.p2.x, dt.p2.y)));
                ++mstSize;
            }
            else {
                // for( it = cycles[trgTag].begin(), itEnd =
                // cycles[trgTag].end(); it != itEnd; ++it )
                // for( auto it : cycles[trgTag] )
                for (auto it = cycles[trgTag].begin(); it != cycles[trgTag].end(); ++it) {
                    tags[aNodes[*it].id] = srcTag;
                    aNodes[*it].tag = srcTag;
                }

                // Processing a connection, decrease the expected size of the
                // ratsnest MST
                --mstExpectedSize;
            }

            // Move nodes that were marked with old tag to the list marked with
            // the new tag
            cycles[srcTag].splice(cycles[srcTag].end(), cycles[trgTag]);
        }

        // Remove the edge that was just processed
        aEdges.pop_back();
    }

    return mst;
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardAirWiresBuilder::BoardAirWiresBuilder(const Board& board, const NetSignal& netsignal) noexcept :
    mBoard(board), mNetSignal(netsignal)
{
}

BoardAirWiresBuilder::~BoardAirWiresBuilder() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QVector<QPair<Point, Point> > BoardAirWiresBuilder::buildAirWires() const
{
    std::vector<delaunay::Vector2<qreal>> points;
    QHash<const BI_NetLineAnchor*, int> anchorMap;
    QHash<int, QString> layerMap;
    std::vector<delaunay::Edge<qreal>> edges;

    // pads
    foreach (ComponentSignalInstance* cmpSig, mNetSignal.getComponentSignals()) { Q_ASSERT(cmpSig);
        foreach (BI_FootprintPad* pad, cmpSig->getRegisteredFootprintPads()) {
            if (&pad->getBoard() != &mBoard) continue;
            int id = points.size();
            Point pos = pad->getPosition();
            points.emplace_back(pos.getX().toNm(), pos.getY().toNm(), id);
            anchorMap[pad] = id;
            if (pad->getLibPad().getBoardSide() == library::FootprintPad::BoardSide::THT) {
                layerMap[id] = QString(); // on all layers
            } else {
                layerMap[id] = pad->getLayerName();
            }
        }
    }

    // vias, netpoints, netlines
    foreach (const BI_NetSegment* netsegment, mNetSignal.getBoardNetSegments()) { Q_ASSERT(netsegment);
        if (&netsegment->getBoard() != &mBoard) continue;
        foreach (const BI_Via* via, netsegment->getVias()) { Q_ASSERT(via);
            int id = points.size();
            Point pos = via->getPosition();
            points.emplace_back(pos.getX().toNm(), pos.getY().toNm(), id);
            anchorMap[via] = id;
            layerMap[id] = QString(); // on all layers
        }
        foreach (const BI_NetPoint* netpoint, netsegment->getNetPoints()) { Q_ASSERT(netpoint);
            if (const GraphicsLayer* layer = netpoint->getLayerOfLines()) {
                int id = points.size();
                Point pos = netpoint->getPosition();
                points.emplace_back(pos.getX().toNm(), pos.getY().toNm(), id);
                anchorMap[netpoint] = id;
                layerMap[id] = layer->getName();
            }
        }
        foreach (const BI_NetLine* netline, netsegment->getNetLines()) { Q_ASSERT(netline);
            Q_ASSERT(anchorMap.contains(&netline->getStartPoint()));
            Q_ASSERT(anchorMap.contains(&netline->getEndPoint()));
            edges.emplace_back(points[anchorMap[&netline->getStartPoint()]],
                               points[anchorMap[&netline->getEndPoint()]], -1);
        }
    }

    // determine connections made by planes
    foreach (const BI_Plane* plane, mNetSignal.getBoardPlanes()) { Q_ASSERT(plane);
        if (&plane->getBoard() != &mBoard) continue;
        foreach (const Path& fragment, plane->getFragments()) {
            int lastId = -1;
            for (const auto& point : points) {
                QString pointLayer = layerMap[point.id];
                if (pointLayer.isNull() || (pointLayer == plane->getLayerName())) {
                    Point p(point.x, point.y);
                    if (fragment.toQPainterPathPx().contains(p.toPxQPointF())) {
                        if (lastId >= 0) {
                            edges.emplace_back(points[lastId], points[point.id], -1);
                        }
                        lastId = point.id;
                    }
                }
            }
        }
    }

    // remember how many edges are already known as connected
    uint connectedEdges = edges.size();

    // determine additional edges between found points (candidates for airwires)
    if (points.size() >= 3) { // minimum 3 points needed for triangulation
        delaunay::Delaunay<qreal> del;
        del.triangulate(points);
        edges.insert(edges.end(), del.getEdges().begin(), del.getEdges().end());
    } else if (points.size() == 2) {
        edges.emplace_back(points[0], points[1], -1);
    }

    // determine weights of these new edges
    for (uint i = connectedEdges; i < edges.size(); ++i) {
        edges[i].weight = edges[i].p1.dist2(edges[i].p2);
    }

    // find airwires in list of edges
    return kruskalMst(edges, points);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
