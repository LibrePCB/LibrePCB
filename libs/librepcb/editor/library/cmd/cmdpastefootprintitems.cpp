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
#include "cmdpastefootprintitems.h"

#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdholeedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdstroketextedit.h"
#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/holegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/stroketextgraphicsitem.h"
#include "../pkg/footprintclipboarddata.h"
#include "../pkg/footprintgraphicsitem.h"
#include "../pkg/footprintpadgraphicsitem.h"
#include "cmdfootprintpadedit.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPasteFootprintItems::CmdPasteFootprintItems(
    Package& package, Footprint& footprint, FootprintGraphicsItem& graphicsItem,
    std::unique_ptr<FootprintClipboardData> data,
    const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Footprint Elements")),
    mPackage(package),
    mFootprint(footprint),
    mGraphicsItem(graphicsItem),
    mData(std::move(data)),
    mPosOffset(posOffset) {
  Q_ASSERT(mData);
}

CmdPasteFootprintItems::~CmdPasteFootprintItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPasteFootprintItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Notes:
  //
  //  - If the UUID is already existing, or the destination footprint is
  //    different to the source footprint, generate a new random UUID. Otherwise
  //    use the same UUID to avoid modifications after cut+paste within one
  //    footprint.
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  for (const FootprintPad& pad : mData->getFootprintPads().sortedByUuid()) {
    Uuid uuid = pad.getUuid();
    if (mFootprint.getPads().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<PackagePad> pkgPad;
    if (pad.getPackagePadUuid()) {
      pkgPad = mData->getPackagePads().get(*pad.getPackagePadUuid());
    }
    if (pkgPad) {
      pkgPad = mPackage.getPads().find(*pkgPad->getName());
    }
    tl::optional<Uuid> pkgPadUuid =
        pkgPad ? pkgPad->getUuid() : tl::optional<Uuid>();
    std::shared_ptr<FootprintPad> copy = std::make_shared<FootprintPad>(
        uuid, pkgPadUuid, pad.getPosition() + mPosOffset, pad.getRotation(),
        pad.getShape(), pad.getWidth(), pad.getHeight(), pad.getRadius(),
        pad.getCustomShapeOutline(), pad.getStopMaskConfig(),
        pad.getSolderPasteConfig(), pad.getCopperClearance(),
        pad.getComponentSide(), pad.getFunction(), pad.getHoles());
    execNewChildCmd(new CmdFootprintPadInsert(mFootprint.getPads(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select pad graphics item!";
    }
  }

  for (const Circle& circle : mData->getCircles().sortedByUuid()) {
    Uuid uuid = circle.getUuid();
    if (mFootprint.getCircles().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Circle> copy = std::make_shared<Circle>(
        uuid, circle.getLayer(), circle.getLineWidth(), circle.isFilled(),
        circle.isGrabArea(), circle.getCenter() + mPosOffset,
        circle.getDiameter());
    execNewChildCmd(new CmdCircleInsert(mFootprint.getCircles(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select circle graphics item!";
    }
  }

  for (const Polygon& polygon : mData->getPolygons().sortedByUuid()) {
    Uuid uuid = polygon.getUuid();
    if (mFootprint.getPolygons().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Polygon> copy = std::make_shared<Polygon>(
        uuid, polygon.getLayer(), polygon.getLineWidth(), polygon.isFilled(),
        polygon.isGrabArea(), polygon.getPath().translated(mPosOffset));
    execNewChildCmd(new CmdPolygonInsert(mFootprint.getPolygons(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select polygon graphics item!";
    }
  }

  for (const StrokeText& text : mData->getStrokeTexts().sortedByUuid()) {
    Uuid uuid = text.getUuid();
    if (mFootprint.getStrokeTexts().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<StrokeText> copy = std::make_shared<StrokeText>(
        uuid, text.getLayer(), text.getText(), text.getPosition() + mPosOffset,
        text.getRotation(), text.getHeight(), text.getStrokeWidth(),
        text.getLetterSpacing(), text.getLineSpacing(), text.getAlign(),
        text.getMirrored(), text.getAutoRotate());
    execNewChildCmd(new CmdStrokeTextInsert(mFootprint.getStrokeTexts(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select stroke text graphics item!";
    }
  }

  for (const Hole& hole : mData->getHoles().sortedByUuid()) {
    Uuid uuid = hole.getUuid();
    if (mFootprint.getHoles().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Hole> copy = std::make_shared<Hole>(
        uuid, hole.getDiameter(),
        NonEmptyPath(hole.getPath()->translated(mPosOffset)),
        hole.getStopMaskConfig());
    execNewChildCmd(new CmdHoleInsert(mFootprint.getHoles(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select hole graphics item!";
    }
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
