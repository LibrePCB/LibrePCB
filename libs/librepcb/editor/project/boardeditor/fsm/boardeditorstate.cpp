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

#include "../../../graphics/graphicslayer.h"
#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../boardeditor.h"
#include "../boardgraphicsscene.h"
#include "../graphicsitems/bgi_device.h"
#include "../graphicsitems/bgi_footprintpad.h"
#include "../graphicsitems/bgi_hole.h"
#include "../graphicsitems/bgi_netline.h"
#include "../graphicsitems/bgi_netpoint.h"
#include "../graphicsitems/bgi_plane.h"
#include "../graphicsitems/bgi_polygon.h"
#include "../graphicsitems/bgi_stroketext.h"
#include "../graphicsitems/bgi_via.h"
#include "../graphicsitems/bgi_zone.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/project/board/board.h>
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
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/utils/toolbox.h>
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

BoardGraphicsScene* BoardEditorState::getActiveBoardScene() noexcept {
  return mContext.editor.getActiveBoardScene();
}

bool BoardEditorState::getIgnoreLocks() const noexcept {
  return mContext.editor.getIgnoreLocks();
}

PositiveLength BoardEditorState::getGridInterval() const noexcept {
  return mContext.editorGraphicsView.getGridInterval();
}

const LengthUnit& BoardEditorState::getLengthUnit() const noexcept {
  if (const Board* board =
          const_cast<BoardEditorState*>(this)->getActiveBoard()) {
    return board->getGridUnit();
  } else {
    return mContext.workspace.getSettings().defaultLengthUnit.get();
  }
}

QSet<const Layer*> BoardEditorState::getAllowedGeometryLayers() noexcept {
  static const QSet<const Layer*> commonLayers = {
      &Layer::boardSheetFrames(),
      &Layer::boardOutlines(),
      &Layer::boardCutouts(),
      &Layer::boardPlatedCutouts(),
      &Layer::boardMeasures(),
      &Layer::boardAlignment(),
      &Layer::boardDocumentation(),
      &Layer::boardComments(),
      &Layer::boardGuide(),
      &Layer::topNames(),
      &Layer::topValues(),
      &Layer::topLegend(),
      &Layer::topDocumentation(),
      // &Layer::topPackageOutlines(), -> makes no sense in boards
      // &Layer::topCourtyard(), -> makes no sense in boards
      // &Layer::topHiddenGrabAreas(), -> makes no sense in boards
      &Layer::topCopper(),
      &Layer::topGlue(),
      &Layer::topSolderPaste(),
      &Layer::topStopMask(),
      &Layer::botNames(),
      &Layer::botValues(),
      &Layer::botLegend(),
      &Layer::botDocumentation(),
      // &Layer::botPackageOutlines(), -> makes no sense in boards
      // &Layer::botCourtyard(), -> makes no sense in boards
      // &Layer::botHiddenGrabAreas(), -> makes no sense in boards
      &Layer::botCopper(),
      &Layer::botGlue(),
      &Layer::botSolderPaste(),
      &Layer::botStopMask(),
  };
  QSet<const Layer*> layers = commonLayers;
  if (const Board* board = getActiveBoard()) {
    layers |= board->getCopperLayers();
  }
  return layers;
}

void BoardEditorState::makeLayerVisible(const QString& layer) noexcept {
  if (std::shared_ptr<GraphicsLayer> l = mContext.editor.getLayer(layer)) {
    if (l->isEnabled()) {
      l->setVisible(true);
    }
  }
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

QList<std::shared_ptr<QGraphicsItem>> BoardEditorState::findItemsAtPos(
    const Point& pos, FindFlags flags, const Layer* cuLayer,
    const QSet<const NetSignal*>& netsignals,
    const QVector<std::shared_ptr<QGraphicsItem>>& except) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) {
    return QList<std::shared_ptr<QGraphicsItem>>();
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
  //     1: pads THT
  //     5: holes
  //    50: polygons/texts board layer
  //   110: netpoints top
  //   120: netlines top
  //   130: planes/zones top
  //   140: footprints top
  //   150: pads top
  //   160: polygons/texts top
  //   210: netpoints inner
  //   220: netlines inner
  //   230: planes/zones inner
  //   240: polygons/texts inner
  //   310: netpoints bottom
  //   320: netlines bottom
  //   330: planes/zones bottom
  //   340: footprints bottom
  //   350: pads bottom
  //   360: polygons/texts bottom
  //
  // So the system is:
  //      0 for vias
  //      5 for holes
  //     10 for netpoints
  //     20 for netlines
  //     30 for planes/zones
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
  QMultiMap<std::pair<int, int>, std::shared_ptr<QGraphicsItem>> items;
  std::optional<std::pair<int, int>> lowestPriority;
  auto addItem = [&items, &lowestPriority](
                     const std::pair<int, int>& prio,
                     std::shared_ptr<QGraphicsItem> item) {
    if ((!lowestPriority) || (prio < (*lowestPriority))) {
      lowestPriority = prio;
    }
    items.insert(prio, item);
  };
  auto canSkip = [&lowestPriority, flags](const std::pair<int, int>& prio) {
    return flags.testFlag(FindFlag::SkipLowerPriorityMatches) &&
        lowestPriority && (prio > (*lowestPriority));
  };
  auto priorityFromLayer = [](const Layer& layer) {
    if (layer.isTop()) {
      return 100;
    } else if (layer.isInner()) {
      return 200;
    } else if (layer.isBottom()) {
      return 300;
    } else {
      return 0;
    }
  };
  auto processItem = [&pos, &posExact, &posOnGrid, &posArea, &posAreaLarge,
                      flags, &except, &addItem, &canSkip](
                         std::shared_ptr<QGraphicsItem> item,
                         const Point& nearestPos, int priority, bool large) {
    if (except.contains(item)) {
      return;
    }
    auto prio = std::make_pair(priority, 0);
    if (canSkip(prio)) {
      return;
    }
    const QPainterPath grabArea = item->mapToScene(item->shape());
    if (grabArea.isEmpty()) {
      return;
    }
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
    for (auto it = scene->getHoles().begin(); it != scene->getHoles().end();
         it++) {
      processItem(it.value(),
                  it.key()->getData().getPath()->getVertices().first().getPos(),
                  5, false);
    }
  }

  if (flags.testFlag(FindFlag::Vias)) {
    for (auto it = scene->getVias().begin(); it != scene->getVias().end();
         it++) {
      if (netsignals.isEmpty() ||
          netsignals.contains(it.key()->getNetSegment().getNetSignal())) {
        if ((!cuLayer) || (it.key()->getVia().isOnLayer(*cuLayer))) {
          processItem(it.value(), it.key()->getPosition(), 0, false);
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::NetPoints)) {
    for (auto it = scene->getNetPoints().begin();
         it != scene->getNetPoints().end(); it++) {
      if (netsignals.isEmpty() ||
          netsignals.contains(it.key()->getNetSegment().getNetSignal())) {
        const Layer* layer = it.key()->getLayerOfTraces();
        if ((!cuLayer) || (&*cuLayer == layer)) {
          processItem(it.value(), it.key()->getPosition(),
                      10 + (layer ? priorityFromLayer(*layer) : 0), false);
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::NetLines)) {
    for (auto it = scene->getNetLines().begin();
         it != scene->getNetLines().end(); it++) {
      if (netsignals.isEmpty() ||
          netsignals.contains(it.key()->getNetSegment().getNetSignal())) {
        const Layer& layer = it.key()->getLayer();
        if ((!cuLayer) || (*cuLayer == layer)) {
          processItem(it.value(),
                      Toolbox::nearestPointOnLine(
                          pos.mappedToGrid(getGridInterval()),
                          it.key()->getStartPoint().getPosition(),
                          it.key()->getEndPoint().getPosition()),
                      20 + priorityFromLayer(layer), false);
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Planes)) {
    for (auto it = scene->getPlanes().begin(); it != scene->getPlanes().end();
         it++) {
      if (netsignals.isEmpty() ||
          netsignals.contains(it.key()->getNetSignal())) {
        if ((!cuLayer) || (*cuLayer == it.key()->getLayer())) {
          processItem(
              it.value(),
              it.key()->getOutline().calcNearestPointBetweenVertices(pos),
              30 + priorityFromLayer(it.key()->getLayer()),
              true);  // Probably large grab area makes sense?
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Zones)) {
    for (auto it = scene->getZones().begin(); it != scene->getZones().end();
         it++) {
      if ((!cuLayer) || (it.key()->getData().getLayers().contains(&*cuLayer))) {
        QList<const Layer*> layers =
            Toolbox::toList(it.key()->getData().getLayers());
        std::sort(layers.begin(), layers.end(), &Layer::lessThan);
        int priority = 30;
        if (!layers.isEmpty()) {
          priority += priorityFromLayer(*layers.first());
        }
        processItem(
            it.value(),
            it.key()->getData().getOutline().calcNearestPointBetweenVertices(
                pos),
            priority,
            true);  // Probably large grab area makes sense?
      }
    }
  }

  if (flags.testFlag(FindFlag::Devices)) {
    for (auto it = scene->getDevices().begin(); it != scene->getDevices().end();
         it++) {
      processItem(it.value(), it.key()->getPosition(),
                  40 + (it.key()->getMirrored() ? 300 : 100), false);
    }
  }

  if (flags.testFlag(FindFlag::FootprintPads)) {
    for (auto it = scene->getFootprintPads().begin();
         it != scene->getFootprintPads().end(); it++) {
      if (netsignals.isEmpty() ||
          netsignals.contains(it.key()->getCompSigInstNetSignal())) {
        if ((!cuLayer) || (it.key()->isOnLayer(*cuLayer))) {
          // Give THT pads high priority to fix
          // https://github.com/LibrePCB/LibrePCB/issues/1073.
          const int priority = it.key()->getLibPad().isTht()
              ? 1
              : (50 + (it.key()->getMirrored() ? 300 : 100));
          processItem(it.value(), it.key()->getPosition(), priority, false);
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Polygons)) {
    for (auto it = scene->getPolygons().begin();
         it != scene->getPolygons().end(); it++) {
      processItem(
          it.value(),
          it.key()->getData().getPath().calcNearestPointBetweenVertices(pos),
          60 + priorityFromLayer(it.key()->getData().getLayer()),
          true);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::StrokeTexts)) {
    for (auto it = scene->getStrokeTexts().begin();
         it != scene->getStrokeTexts().end(); it++) {
      processItem(it.value(), it.key()->getData().getPosition(),
                  60 + priorityFromLayer(it.key()->getData().getLayer()),
                  false);
    }
  }

  return items.values();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
