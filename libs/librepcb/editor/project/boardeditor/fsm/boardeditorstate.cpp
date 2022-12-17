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
#include "boardeditorstate.h"

#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../boardeditor.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/types/gridproperties.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState::BoardEditorState(const Context& context,
                                   QObject* parent) noexcept
  : QObject(parent), mContext(context) {
}

BoardEditorState::~BoardEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

Board* BoardEditorState::getActiveBoard() noexcept {
  return mContext.editor.getActiveBoard();
}

PositiveLength BoardEditorState::getGridInterval() const noexcept {
  return mContext.editorGraphicsView.getGridProperties().getInterval();
}

const LengthUnit& BoardEditorState::getDefaultLengthUnit() const noexcept {
  return mContext.workspace.getSettings().defaultLengthUnit.get();
}

QList<GraphicsLayer*> BoardEditorState::getAllowedGeometryLayers(
    const Board& board) const noexcept {
  return board.getLayerStack().getLayers({
      GraphicsLayer::sBoardSheetFrames,
      GraphicsLayer::sBoardOutlines,
      GraphicsLayer::sBoardMillingPth,
      GraphicsLayer::sBoardMeasures,
      GraphicsLayer::sBoardAlignment,
      GraphicsLayer::sBoardDocumentation,
      GraphicsLayer::sBoardComments,
      GraphicsLayer::sBoardGuide,
      GraphicsLayer::sTopPlacement,
      // GraphicsLayer::sTopHiddenGrabAreas, -> makes no sense in boards
      GraphicsLayer::sTopDocumentation,
      GraphicsLayer::sTopNames,
      GraphicsLayer::sTopValues,
      GraphicsLayer::sTopCopper,
      GraphicsLayer::sTopCourtyard,
      GraphicsLayer::sTopGlue,
      GraphicsLayer::sTopSolderPaste,
      GraphicsLayer::sTopStopMask,
      GraphicsLayer::sBotPlacement,
      // GraphicsLayer::sBotHiddenGrabAreas, -> makes no sense in boards
      GraphicsLayer::sBotDocumentation,
      GraphicsLayer::sBotNames,
      GraphicsLayer::sBotValues,
      GraphicsLayer::sBotCopper,
      GraphicsLayer::sBotCourtyard,
      GraphicsLayer::sBotGlue,
      GraphicsLayer::sBotSolderPaste,
      GraphicsLayer::sBotStopMask,
  });
}

void BoardEditorState::abortBlockingToolsInOtherEditors() noexcept {
  mContext.editor.abortBlockingToolsInOtherEditors();
}

bool BoardEditorState::execCmd(UndoCommand* cmd) {
  return mContext.undoStack.execCmd(cmd);
}

QWidget* BoardEditorState::parentWidget() noexcept {
  return &mContext.editor;
}

QList<BI_Base*> BoardEditorState::findItemsAtPos(
    const Point& pos, FindFlags flags, const GraphicsLayer* cuLayer,
    const QSet<const NetSignal*>& netsignals,
    const QSet<BI_Base*>& except) noexcept {
  Board* board = getActiveBoard();
  if (!board) {
    return QList<BI_Base*>();
  }

  const QPointF posExact = pos.toPxQPointF();
  const QPointF posOnGrid = pos.mappedToGrid(getGridInterval()).toPxQPointF();
  const QPainterPath posArea =
      mContext.editorGraphicsView.calcPosWithTolerance(pos);
  const QPainterPath posAreaLarge =
      mContext.editorGraphicsView.calcPosWithTolerance(pos, 1.5);

  // Note: The order of adding the items is very important (the top most item
  // must appear as the first item in the list)! For that, we work with
  // priorities (0 = highest priority):
  //
  //     0: vias
  //     5: holes
  //    50: polygons/texts board layer
  //   110: netpoints top
  //   120: netlines top
  //   130: planes top
  //   140: footprints top
  //   150: pads top
  //   160: polygons/texts top
  //   210: netpoints inner
  //   220: netlines inner
  //   230: planes inner
  //   240: polygons/texts inner
  //   310: netpoints bottom
  //   320: netlines bottom
  //   330: planes bottom
  //   340: footprints bottom
  //   350: pads bottom
  //   360: polygons/texts bottom
  //
  // So the system is:
  //      0 for vias
  //      5 for holes
  //     10 for netpoints
  //     20 for netlines
  //     30 for planes
  //     40 for footprints
  //     50 for pads
  //     60 for polygons/texts
  //   +100 for top layer items
  //   +200 for inner layer items
  //   +300 for bottom layer items
  //
  // And for items not directly under the cursor, but very close to the cursor,
  // add +1000. For items not under the cursor, but on the next grid interval,
  // add +2000.
  QMultiMap<std::pair<int, int>, BI_Base*> items;
  tl::optional<std::pair<int, int>> lowestPriority;
  auto addItem = [&items, &lowestPriority](const std::pair<int, int>& prio,
                                           BI_Base* item) {
    if ((!lowestPriority) || (prio < (*lowestPriority))) {
      lowestPriority = prio;
    }
    items.insert(prio, item);
  };
  auto canSkip = [&lowestPriority, flags](const std::pair<int, int>& prio) {
    return flags.testFlag(FindFlag::SkipLowerPriorityMatches) &&
        lowestPriority && (prio > (*lowestPriority));
  };
  auto priorityFromLayer = [](const QString& layerName) {
    if (GraphicsLayer::isTopLayer(layerName)) {
      return 100;
    } else if (GraphicsLayer::isInnerLayer(layerName)) {
      return 200;
    } else if (GraphicsLayer::isBottomLayer(layerName)) {
      return 300;
    } else {
      return 0;
    }
  };
  auto processItem = [&pos, &posExact, &posOnGrid, &posArea, &posAreaLarge,
                      flags, &except, &addItem,
                      &canSkip](BI_Base* item, const Point& nearestPos,
                                int priority, bool large = false) {
    if (except.contains(item) || (!item->isSelectable())) {
      return;
    }
    auto prio = std::make_pair(priority, 0);
    if (canSkip(prio)) {
      return;
    }
    const QPainterPath grabArea = item->getGrabAreaScenePx();
    const int distance = qRound((nearestPos - pos).getLength()->toPx());
    prio = std::make_pair(priority, distance);
    if (canSkip(prio)) {
      return;
    }
    if (grabArea.contains(posExact)) {
      addItem(prio, item);
      return;
    }
    prio = std::make_pair(priority + 1000, distance);
    if (canSkip(prio)) {
      return;
    }
    if ((flags & (FindFlag::AcceptNearMatch | FindFlag::AcceptNextGridMatch)) &&
        grabArea.intersects(large ? posAreaLarge : posArea)) {
      addItem(prio, item);
      return;
    }
    prio = std::make_pair(distance + 2000, priority);  // Swapped order!
    if (canSkip(prio)) {
      return;
    }
    if ((flags & FindFlag::AcceptNextGridMatch) && (posOnGrid != posExact) &&
        grabArea.contains(posOnGrid)) {
      addItem(prio, item);
      return;
    }
  };

  if (flags.testFlag(FindFlag::Holes)) {
    foreach (BI_Hole* hole, board->getHoles()) {
      processItem(hole,
                  hole->getHole().getPath()->getVertices().first().getPos(), 5);
    }
  }

  if (flags & (FindFlag::Vias | FindFlag::NetPoints | FindFlag::NetLines)) {
    foreach (BI_NetSegment* segment, board->getNetSegments()) {
      if ((!netsignals.isEmpty()) &&
          (!netsignals.contains(segment->getNetSignal()))) {
        continue;
      }
      if (flags.testFlag(FindFlag::Vias)) {
        foreach (BI_Via* via, segment->getVias()) {
          processItem(via, via->getPosition(), 0);
        }
      }
      if (flags.testFlag(FindFlag::NetPoints)) {
        foreach (BI_NetPoint* netpoint, segment->getNetPoints()) {
          const GraphicsLayer* layer = netpoint->getLayerOfLines();
          if (cuLayer && (layer != cuLayer)) {
            continue;
          }
          processItem(netpoint, netpoint->getPosition(),
                      10 + (layer ? priorityFromLayer(layer->getName()) : 0));
        }
      }
      if (flags.testFlag(FindFlag::NetLines)) {
        foreach (BI_NetLine* netline, segment->getNetLines()) {
          const GraphicsLayer& layer = netline->getLayer();
          if (cuLayer && (&layer != cuLayer)) {
            continue;
          }
          processItem(netline,
                      Toolbox::nearestPointOnLine(
                          pos.mappedToGrid(getGridInterval()),
                          netline->getStartPoint().getPosition(),
                          netline->getEndPoint().getPosition()),
                      20 + priorityFromLayer(layer.getName()));
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Planes)) {
    foreach (BI_Plane* plane, board->getPlanes()) {
      if ((!netsignals.isEmpty()) &&
          (!netsignals.contains(&plane->getNetSignal()))) {
        continue;
      }
      if (cuLayer && (*plane->getLayerName() != cuLayer->getName())) {
        continue;
      }
      processItem(plane,
                  plane->getOutline().calcNearestPointBetweenVertices(pos),
                  30 + priorityFromLayer(*plane->getLayerName()),
                  true);  // Probably large grab area makes sense?
    }
  }

  if (flags &
      (FindFlag::Footprints | FindFlag::FootprintPads |
       FindFlag::StrokeTexts)) {
    foreach (BI_Device* device, board->getDeviceInstances()) {
      if (flags.testFlag(FindFlag::Footprints)) {
        processItem(device, device->getPosition(),
                    40 + (device->getMirrored() ? 300 : 100));
      }
      if (flags.testFlag(FindFlag::FootprintPads)) {
        foreach (BI_FootprintPad* pad, device->getPads()) {
          if ((!netsignals.isEmpty()) &&
              (!netsignals.contains(pad->getCompSigInstNetSignal()))) {
            continue;
          }
          if (cuLayer && (!pad->isOnLayer(cuLayer->getName()))) {
            continue;
          }
          processItem(pad, pad->getPosition(),
                      50 + (pad->getMirrored() ? 300 : 100));
        }
      }
      if (flags.testFlag(FindFlag::StrokeTexts)) {
        foreach (BI_StrokeText* text, device->getStrokeTexts()) {
          processItem(text, text->getPosition(),
                      60 + priorityFromLayer(*text->getText().getLayerName()));
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Polygons)) {
    foreach (BI_Polygon* polygon, board->getPolygons()) {
      processItem(
          polygon,
          polygon->getPolygon().getPath().calcNearestPointBetweenVertices(pos),
          60 + priorityFromLayer(*polygon->getPolygon().getLayerName()),
          true);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::StrokeTexts)) {
    foreach (BI_StrokeText* text, board->getStrokeTexts()) {
      processItem(text, text->getPosition(),
                  60 + priorityFromLayer(*text->getText().getLayerName()));
    }
  }

  return items.values();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
