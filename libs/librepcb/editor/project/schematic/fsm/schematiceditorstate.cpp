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
#include "schematiceditorstate.h"

#include "../../../graphics/imagegraphicsitem.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../graphicsitems/sgi_netlabel.h"
#include "../graphicsitems/sgi_netline.h"
#include "../graphicsitems/sgi_netpoint.h"
#include "../graphicsitems/sgi_symbol.h"
#include "../graphicsitems/sgi_symbolpin.h"
#include "../graphicsitems/sgi_text.h"
#include "../schematicgraphicsscene.h"
#include "schematiceditorfsmadapter.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_image.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/layer.h>
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

SchematicEditorState::SchematicEditorState(const Context& context,
                                           QObject* parent) noexcept
  : QObject(parent), mContext(context), mAdapter(context.adapter) {
}

SchematicEditorState::~SchematicEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

SchematicGraphicsScene*
    SchematicEditorState::getActiveSchematicScene() noexcept {
  return mAdapter.fsmGetGraphicsScene();
}

PositiveLength SchematicEditorState::getGridInterval() const noexcept {
  return mContext.schematic.getGridInterval();
}

const LengthUnit& SchematicEditorState::getLengthUnit() const noexcept {
  return mContext.schematic.getGridUnit();
}

const QSet<const Layer*>&
    SchematicEditorState::getAllowedGeometryLayers() noexcept {
  static QSet<const Layer*> layers = {
      &Layer::symbolOutlines(),
      // &Layer::symbolHiddenGrabAreas(), -> makes no sense in schematics
      &Layer::symbolNames(),
      &Layer::symbolValues(),
      &Layer::schematicSheetFrames(),
      &Layer::schematicDocumentation(),
      &Layer::schematicComments(),
      &Layer::schematicGuide(),
  };
  return layers;
}

void SchematicEditorState::abortBlockingToolsInOtherEditors() noexcept {
  mAdapter.fsmAbortBlockingToolsInOtherEditors();
}

bool SchematicEditorState::execCmd(UndoCommand* cmd) {
  return mContext.undoStack.execCmd(cmd);
}

QWidget* SchematicEditorState::parentWidget() noexcept {
  return qApp->activeWindow();
}

QList<std::shared_ptr<QGraphicsItem>> SchematicEditorState::findItemsAtPos(
    const Point& pos, FindFlags flags,
    const QVector<std::shared_ptr<QGraphicsItem>>& except) noexcept {
  SchematicGraphicsScene* scene = getActiveSchematicScene();
  if (!scene) {
    return QList<std::shared_ptr<QGraphicsItem>>();
  }

  const QPointF posExact = pos.toPxQPointF();
  const QPainterPath posArea = mAdapter.fsmCalcPosWithTolerance(pos, 1);
  const QPainterPath posAreaLarge = mAdapter.fsmCalcPosWithTolerance(pos, 2);

  QPainterPath posAreaInGrid;
  const Point posOnGrid = pos.mappedToGrid(getGridInterval());
  if (posOnGrid != pos) {
    const qreal gridDistancePx = (pos - posOnGrid).getLength()->toPx() +
        (getGridInterval()->toPx() / 100);
    posAreaInGrid.addEllipse(pos.toPxQPointF(), gridDistancePx, gridDistancePx);
  }

  // Note: The order of adding the items is very important (the top most item
  // must appear as the first item in the list)! For that, we work with
  // priorities (0 = highest priority):
  //
  //    0: visible netpoints
  //   10: hidden netpoints
  //   20: netlines
  //   30: netlabels
  //   40: pins
  //   50: symbols with origin close to cursor
  //   60: texts
  //   70: symbols with any grab area below cursor
  //   80: polygons
  //   90: images
  //
  // And for items not directly under the cursor, but very close to the cursor,
  // add +1000. For items not under the cursor, but on the next grid interval,
  // add +2000.
  //
  // Note regarding priority of symbols and texts: Although texts are drawn on
  // top of symbols, selection order must be the other way around when clicking
  // on the origin of a symbol. Otherwise "zero-area" symbols like GND or VCC
  // with a text at position (0,0) can't be selected because the text gets
  // selected instead (which is very cumbersome).
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
  auto processItem = [&pos, &posExact, &posArea, &posAreaLarge, &posAreaInGrid,
                      flags, &except, &addItem, &canSkip](
                         std::shared_ptr<QGraphicsItem> item,
                         std::shared_ptr<QGraphicsItem> itemToAdd,
                         const Point& nearestPos, int priority, bool large,
                         const std::optional<UnsignedLength>& maxDistance) {
    if (except.contains(itemToAdd)) {
      return false;
    }
    auto prio = std::make_pair(priority, 0);
    if (canSkip(prio)) {
      return false;
    }
    const QPainterPath grabArea = item->mapToScene(item->shape());
    const UnsignedLength distance = (nearestPos - pos).getLength();
    if ((maxDistance) && (distance > (*maxDistance))) {
      return false;
    }
    const int distanceInt = qRound(distance->toPx());
    prio = std::make_pair(priority, distanceInt);
    if (canSkip(prio)) {
      return false;
    }
    if (grabArea.contains(posExact)) {
      addItem(prio, itemToAdd);
      return true;
    }
    prio = std::make_pair(priority + 1000, distanceInt);
    if (canSkip(prio)) {
      return false;
    }
    if ((flags &
         (FindFlag::AcceptNearMatch | FindFlag::AcceptNearestWithinGrid)) &&
        grabArea.intersects(large ? posAreaLarge : posArea)) {
      addItem(prio, itemToAdd);
      return true;
    }
    prio = std::make_pair(distanceInt + 2000, priority);  // Swapped order!
    if (canSkip(prio)) {
      return false;
    }
    if ((flags & FindFlag::AcceptNearestWithinGrid) &&
        (!posAreaInGrid.isEmpty()) && grabArea.intersects(posAreaInGrid)) {
      addItem(prio, itemToAdd);
      return true;
    }
    return false;
  };

  if (flags.testFlag(FindFlag::NetPoints)) {
    for (auto it = scene->getNetPoints().begin();
         it != scene->getNetPoints().end(); it++) {
      processItem(it.value(), it.value(), it.key()->getPosition(),
                  it.key()->isVisibleJunction() ? 0 : 10, false, std::nullopt);
    }
  }

  if (flags.testFlag(FindFlag::NetLines)) {
    for (auto it = scene->getNetLines().begin();
         it != scene->getNetLines().end(); it++) {
      processItem(
          it.value(), it.value(),
          Toolbox::nearestPointOnLine(pos.mappedToGrid(getGridInterval()),
                                      it.key()->getP1().getPosition(),
                                      it.key()->getP2().getPosition()),
          20, true, std::nullopt);  // Large grab area, better usability!
    }
  }

  if (flags.testFlag(FindFlag::NetLabels)) {
    for (auto it = scene->getNetLabels().begin();
         it != scene->getNetLabels().end(); it++) {
      processItem(it.value(), it.value(), it.key()->getPosition(), 30, false,
                  std::nullopt);
    }
  }

  if (flags.testFlag(FindFlag::Symbols)) {
    for (auto it = scene->getSymbols().begin(); it != scene->getSymbols().end();
         it++) {
      // Higher priority if origin cross is below cursor. Required for
      // https://github.com/LibrePCB/LibrePCB/issues/1319.
      if (!processItem(it.value(), it.value(), it.key()->getPosition(), 50,
                       false, UnsignedLength(700000))) {
        processItem(it.value(), it.value(), it.key()->getPosition(), 70, false,
                    std::nullopt);
      }
    }
  }

  if (flags &
      (FindFlag::SymbolPins | FindFlag::SymbolPinsWithComponentSignal)) {
    for (auto it = scene->getSymbolPins().begin();
         it != scene->getSymbolPins().end(); it++) {
      if (flags.testFlag(FindFlag::SymbolPins) ||
          (it.key()->getComponentSignalInstance())) {
        processItem(it.value(), it.value(), it.key()->getPosition(), 40, false,
                    std::nullopt);
      }
    }
  }

  if (flags.testFlag(FindFlag::Polygons)) {
    for (auto it = scene->getPolygons().begin();
         it != scene->getPolygons().end(); it++) {
      processItem(
          it.value(), it.value(),
          it.key()->getPolygon().getPath().calcNearestPointBetweenVertices(pos),
          80, true, std::nullopt);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::Texts)) {
    for (auto it = scene->getTexts().begin(); it != scene->getTexts().end();
         it++) {
      if ((!it.key()->getTextObj().isLocked()) ||
          mAdapter.fsmGetIgnoreLocks()) {
        processItem(it.value(), it.value(), it.key()->getPosition(), 60, false,
                    std::nullopt);
      } else if (flags.testFlag(FindFlag::Symbols)) {
        // Text is locked, so it cannot be dragged. But if it attached to a
        // symbol, drag the symbol instead, i.e. consider the text as part of
        // the symbols grab area.
        if (auto symItem = it.value()->getSymbolGraphicsItem().lock()) {
          processItem(it.value(), symItem, it.key()->getPosition(), 70, false,
                      std::nullopt);
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Images)) {
    for (auto it = scene->getImages().begin(); it != scene->getImages().end();
         it++) {
      processItem(it.value(), it.value(), it.key()->getPosition(), 90, false,
                  std::nullopt);
    }
  }

  return items.values();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
