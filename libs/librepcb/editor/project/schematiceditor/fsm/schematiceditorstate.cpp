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

#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../schematiceditor.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematiclayerprovider.h>
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
  : QObject(parent), mContext(context) {
}

SchematicEditorState::~SchematicEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

Schematic* SchematicEditorState::getActiveSchematic() noexcept {
  return mContext.editor.getActiveSchematic();
}

PositiveLength SchematicEditorState::getGridInterval() const noexcept {
  return mContext.editorGraphicsView.getGridInterval();
}

const LengthUnit& SchematicEditorState::getLengthUnit() const noexcept {
  if (const Schematic* schematic =
          const_cast<SchematicEditorState*>(this)->getActiveSchematic()) {
    return schematic->getGridUnit();
  } else {
    return mContext.workspace.getSettings().defaultLengthUnit.get();
  }
}

QList<GraphicsLayer*> SchematicEditorState::getAllowedGeometryLayers() const
    noexcept {
  return mContext.project.getLayers().getLayers({
      GraphicsLayer::sSymbolOutlines,
      // GraphicsLayer::sSymbolHiddenGrabAreas, -> makes no sense in schematics
      GraphicsLayer::sSymbolNames,
      GraphicsLayer::sSymbolValues,
      GraphicsLayer::sSchematicSheetFrames,
      GraphicsLayer::sSchematicDocumentation,
      GraphicsLayer::sSchematicComments,
      GraphicsLayer::sSchematicGuide,
  });
}

void SchematicEditorState::abortBlockingToolsInOtherEditors() noexcept {
  mContext.editor.abortBlockingToolsInOtherEditors();
}

bool SchematicEditorState::execCmd(UndoCommand* cmd) {
  return mContext.undoStack.execCmd(cmd);
}

QWidget* SchematicEditorState::parentWidget() noexcept {
  return &mContext.editor;
}

QList<SI_Base*> SchematicEditorState::findItemsAtPos(
    const Point& pos, FindFlags flags, const QSet<SI_Base*>& except) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) {
    return QList<SI_Base*>();
  }

  const QPointF posExact = pos.toPxQPointF();
  const QPainterPath posArea =
      mContext.editorGraphicsView.calcPosWithTolerance(pos);
  const QPainterPath posAreaLarge =
      mContext.editorGraphicsView.calcPosWithTolerance(pos, 2);

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
  //   40: symbols
  //   50: pins
  //   60: polygons
  //   70: texts
  //
  // And for items not directly under the cursor, but very close to the cursor,
  // add +1000. For items not under the cursor, but on the next grid interval,
  // add +2000.
  QMultiMap<std::pair<int, int>, SI_Base*> items;
  tl::optional<std::pair<int, int>> lowestPriority;
  auto addItem = [&items, &lowestPriority](const std::pair<int, int>& prio,
                                           SI_Base* item) {
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
                      flags, &except, &addItem,
                      &canSkip](SI_Base* item, const Point& nearestPos,
                                int priority, bool large = false) {
    if (except.contains(item)) {
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
    if ((flags &
         (FindFlag::AcceptNearMatch | FindFlag::AcceptNearestWithinGrid)) &&
        grabArea.intersects(large ? posAreaLarge : posArea)) {
      addItem(prio, item);
      return;
    }
    prio = std::make_pair(distance + 2000, priority);  // Swapped order!
    if (canSkip(prio)) {
      return;
    }
    if ((flags & FindFlag::AcceptNearestWithinGrid) &&
        (!posAreaInGrid.isEmpty()) && grabArea.intersects(posAreaInGrid)) {
      addItem(prio, item);
      return;
    }
  };

  if (flags &
      (FindFlag::NetPoints | FindFlag::NetLines | FindFlag::NetLabels)) {
    foreach (SI_NetSegment* segment, schematic->getNetSegments()) {
      if (flags.testFlag(FindFlag::NetPoints)) {
        foreach (SI_NetPoint* netpoint, segment->getNetPoints()) {
          processItem(netpoint, netpoint->getPosition(),
                      netpoint->isVisibleJunction() ? 0 : 10);
        }
      }
      if (flags.testFlag(FindFlag::NetLines)) {
        foreach (SI_NetLine* netline, segment->getNetLines()) {
          processItem(netline,
                      Toolbox::nearestPointOnLine(
                          pos.mappedToGrid(getGridInterval()),
                          netline->getStartPoint().getPosition(),
                          netline->getEndPoint().getPosition()),
                      20, true);  // Large grab area, better usability!
        }
      }
      if (flags.testFlag(FindFlag::NetLabels)) {
        foreach (SI_NetLabel* netlabel, segment->getNetLabels()) {
          processItem(netlabel, netlabel->getPosition(), 30);
        }
      }
    }
  }

  if (flags &
      (FindFlag::Symbols | FindFlag::SymbolPins |
       FindFlag::SymbolPinsWithComponentSignal | FindFlag::Texts)) {
    foreach (SI_Symbol* symbol, schematic->getSymbols()) {
      if (flags.testFlag(FindFlag::Symbols)) {
        processItem(symbol, symbol->getPosition(), 40);
      }
      if (flags &
          (FindFlag::SymbolPins | FindFlag::SymbolPinsWithComponentSignal)) {
        foreach (SI_SymbolPin* pin, symbol->getPins()) {
          if (flags.testFlag(FindFlag::SymbolPins) ||
              (pin->getComponentSignalInstance())) {
            processItem(pin, pin->getPosition(), 50);
          }
        }
      }
      if (flags.testFlag(FindFlag::Texts)) {
        foreach (SI_Text* text, symbol->getTexts()) {
          processItem(text, text->getPosition(), 70);
        }
      }
    }
  }

  if (flags.testFlag(FindFlag::Polygons)) {
    foreach (SI_Polygon* polygon, schematic->getPolygons()) {
      processItem(
          polygon,
          polygon->getPolygon().getPath().calcNearestPointBetweenVertices(pos),
          60, true);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::Texts)) {
    foreach (SI_Text* text, schematic->getTexts()) {
      processItem(text, text->getPosition(), 70);
    }
  }

  return items.values();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
