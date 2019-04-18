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

#include "../../footprintclipboarddata.h"

#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/common/scopeguard.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include <librepcb/library/pkg/footprintpadgraphicsitem.h>
#include <librepcb/library/pkg/package.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPasteFootprintItems::CmdPasteFootprintItems(
    Package& package, Footprint& footprint, FootprintGraphicsItem& graphicsItem,
    std::unique_ptr<FootprintClipboardData> data,
    const Point&                            posOffset) noexcept
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
  //  - Footprint pads are only copied if there is an unused package pad with
  //    the same name available.
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  for (const FootprintPad& pad : mData->getFootprintPads().sortedByUuid()) {
    Uuid              uuid = pad.getPackagePadUuid();
    CircuitIdentifier name = mData->getPackagePads().get(uuid)->getName();
    std::shared_ptr<PackagePad> newPad = mPackage.getPads().find(*name);
    if (newPad && (!mFootprint.getPads().contains(newPad->getUuid()))) {
      std::shared_ptr<FootprintPad> copy = std::make_shared<FootprintPad>(
          uuid, pad.getPosition() + mPosOffset, pad.getRotation(),
          pad.getShape(), pad.getWidth(), pad.getHeight(),
          pad.getDrillDiameter(), pad.getBoardSide());
      execNewChildCmd(new CmdFootprintPadInsert(mFootprint.getPads(), copy));
      FootprintPadGraphicsItem* item = mGraphicsItem.getPadGraphicsItem(*copy);
      Q_ASSERT(item);
      item->setSelected(true);
    }
  }

  for (const Circle& circle : mData->getCircles().sortedByUuid()) {
    Uuid uuid = circle.getUuid();
    if (mFootprint.getCircles().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Circle> copy = std::make_shared<Circle>(
        uuid, circle.getLayerName(), circle.getLineWidth(), circle.isFilled(),
        circle.isGrabArea(), circle.getCenter() + mPosOffset,
        circle.getDiameter());
    execNewChildCmd(new CmdCircleInsert(mFootprint.getCircles(), copy));
    CircleGraphicsItem* item = mGraphicsItem.getCircleGraphicsItem(*copy);
    Q_ASSERT(item);
    item->setSelected(true);
  }

  for (const Polygon& polygon : mData->getPolygons().sortedByUuid()) {
    Uuid uuid = polygon.getUuid();
    if (mFootprint.getPolygons().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Polygon> copy = std::make_shared<Polygon>(
        uuid, polygon.getLayerName(), polygon.getLineWidth(),
        polygon.isFilled(), polygon.isGrabArea(),
        polygon.getPath().translated(mPosOffset));
    execNewChildCmd(new CmdPolygonInsert(mFootprint.getPolygons(), copy));
    PolygonGraphicsItem* item = mGraphicsItem.getPolygonGraphicsItem(*copy);
    Q_ASSERT(item);
    item->setSelected(true);
  }

  for (const StrokeText& text : mData->getStrokeTexts().sortedByUuid()) {
    Uuid uuid = text.getUuid();
    if (mFootprint.getStrokeTexts().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<StrokeText> copy = std::make_shared<StrokeText>(
        uuid, text.getLayerName(), text.getText(),
        text.getPosition() + mPosOffset, text.getRotation(), text.getHeight(),
        text.getStrokeWidth(), text.getLetterSpacing(), text.getLineSpacing(),
        text.getAlign(), text.getMirrored(), text.getAutoRotate());
    execNewChildCmd(new CmdStrokeTextInsert(mFootprint.getStrokeTexts(), copy));
    StrokeTextGraphicsItem* item = mGraphicsItem.getTextGraphicsItem(*copy);
    Q_ASSERT(item);
    item->setSelected(true);
  }

  for (const Hole& hole : mData->getHoles().sortedByUuid()) {
    Uuid uuid = hole.getUuid();
    if (mFootprint.getHoles().contains(uuid) ||
        (mFootprint.getUuid() != mData->getFootprintUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Hole> copy = std::make_shared<Hole>(
        uuid, hole.getPosition() + mPosOffset, hole.getDiameter());
    execNewChildCmd(new CmdHoleInsert(mFootprint.getHoles(), copy));
    HoleGraphicsItem* item = mGraphicsItem.getHoleGraphicsItem(*copy);
    Q_ASSERT(item);
    item->setSelected(true);
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
