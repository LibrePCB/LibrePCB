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
#include "cmdpastesymbolitems.h"

#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdtextedit.h"
#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/holegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/textgraphicsitem.h"
#include "../sym/symbolclipboarddata.h"
#include "../sym/symbolgraphicsitem.h"
#include "../sym/symbolpingraphicsitem.h"
#include "cmdsymbolpinedit.h"

#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPasteSymbolItems::CmdPasteSymbolItems(
    Symbol& symbol, SymbolGraphicsItem& graphicsItem,
    std::unique_ptr<SymbolClipboardData> data, const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Symbol Elements")),
    mSymbol(symbol),
    mGraphicsItem(graphicsItem),
    mData(std::move(data)),
    mPosOffset(posOffset) {
  Q_ASSERT(mData);
}

CmdPasteSymbolItems::~CmdPasteSymbolItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPasteSymbolItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Notes:
  //
  //  - If the UUID is already existing, or the destination symbol is different
  //    to the source symbol, generate a new random UUID. Otherwise use the same
  //    UUID to avoid modifications after cut+paste within one symbol.
  //  - If there is already a pin with the same name, increment its number (or
  //    start adding a number if there is none already) to get unique names.
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  QCollator collator;
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  for (const SymbolPin& pin : mData->getPins().sorted(
           [&collator](const SymbolPin& lhs, const SymbolPin& rhs) {
             return collator(*lhs.getName(), *rhs.getName());
           })) {
    Uuid uuid = pin.getUuid();
    if (mSymbol.getPins().contains(uuid) ||
        (mSymbol.getUuid() != mData->getSymbolUuid())) {
      uuid = Uuid::createRandom();
    }
    CircuitIdentifier name = pin.getName();
    for (int i = 0; (i < 1000) && mSymbol.getPins().contains(*name); ++i) {
      name = CircuitIdentifier(
          Toolbox::incrementNumberInString(*name));  // can throw
    }
    std::shared_ptr<SymbolPin> copy = std::make_shared<SymbolPin>(
        uuid, name, pin.getPosition() + mPosOffset, pin.getLength(),
        pin.getRotation(), pin.getNamePosition(), pin.getNameRotation(),
        pin.getNameHeight(), pin.getNameAlignment());
    execNewChildCmd(new CmdSymbolPinInsert(mSymbol.getPins(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select pin graphics item!";
    }
  }

  for (const Circle& circle : mData->getCircles().sortedByUuid()) {
    Uuid uuid = circle.getUuid();
    if (mSymbol.getCircles().contains(uuid) ||
        (mSymbol.getUuid() != mData->getSymbolUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Circle> copy = std::make_shared<Circle>(
        uuid, circle.getLayer(), circle.getLineWidth(), circle.isFilled(),
        circle.isGrabArea(), circle.getCenter() + mPosOffset,
        circle.getDiameter());
    execNewChildCmd(new CmdCircleInsert(mSymbol.getCircles(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select circle graphics item!";
    }
  }

  for (const Polygon& polygon : mData->getPolygons().sortedByUuid()) {
    Uuid uuid = polygon.getUuid();
    if (mSymbol.getPolygons().contains(uuid) ||
        (mSymbol.getUuid() != mData->getSymbolUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Polygon> copy = std::make_shared<Polygon>(
        uuid, polygon.getLayer(), polygon.getLineWidth(), polygon.isFilled(),
        polygon.isGrabArea(), polygon.getPath().translated(mPosOffset));
    execNewChildCmd(new CmdPolygonInsert(mSymbol.getPolygons(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select polygon graphics item!";
    }
  }

  for (const Text& text : mData->getTexts().sortedByUuid()) {
    Uuid uuid = text.getUuid();
    if (mSymbol.getTexts().contains(uuid) ||
        (mSymbol.getUuid() != mData->getSymbolUuid())) {
      uuid = Uuid::createRandom();
    }
    std::shared_ptr<Text> copy = std::make_shared<Text>(
        uuid, text.getLayer(), text.getText(), text.getPosition() + mPosOffset,
        text.getRotation(), text.getHeight(), text.getAlign());
    execNewChildCmd(new CmdTextInsert(mSymbol.getTexts(), copy));
    if (auto graphicsItem = mGraphicsItem.getGraphicsItem(copy)) {
      graphicsItem->setSelected(true);
    } else {
      qCritical() << "Could not select text graphics item!";
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
