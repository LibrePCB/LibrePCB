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
#include "symbolcheck.h"

#include "msg/msgduplicatepinname.h"
#include "msg/msgmissingsymbolname.h"
#include "msg/msgmissingsymbolvalue.h"
#include "msg/msgoverlappingsymbolpins.h"
#include "msg/msgsymbolpinnotongrid.h"
#include "msg/msgwrongsymboltextlayer.h"
#include "symbol.h"

#include <librepcb/common/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolCheck::SymbolCheck(const Symbol& symbol) noexcept
  : LibraryElementCheck(symbol), mSymbol(symbol) {
}

SymbolCheck::~SymbolCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList SymbolCheck::runChecks() const {
  LibraryElementCheckMessageList msgs = LibraryElementCheck::runChecks();
  checkDuplicatePinNames(msgs);
  checkOffTheGridPins(msgs);
  checkOverlappingPins(msgs);
  checkMissingTexts(msgs);
  checkWrongTextLayers(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void SymbolCheck::checkDuplicatePinNames(MsgList& msgs) const {
  QSet<CircuitIdentifier> pinNames;
  for (const SymbolPin& pin : mSymbol.getPins()) {
    if (pinNames.contains(pin.getName())) {
      msgs.append(std::make_shared<MsgDuplicatePinName>(pin));
    } else {
      pinNames.insert(pin.getName());
    }
  }
}

void SymbolCheck::checkOffTheGridPins(MsgList& msgs) const {
  for (auto it = mSymbol.getPins().begin(); it != mSymbol.getPins().end();
       ++it) {
    PositiveLength grid(2540000);
    if (((*it).getPosition() % (*grid)) != Point(0, 0)) {
      msgs.append(std::make_shared<MsgSymbolPinNotOnGrid>(it.ptr(), grid));
    }
  }
}

void SymbolCheck::checkOverlappingPins(MsgList& msgs) const {
  QHash<Point, QVector<std::shared_ptr<const SymbolPin>>> pinPositions;
  for (auto it = mSymbol.getPins().begin(); it != mSymbol.getPins().end();
       ++it) {
    pinPositions[(*it).getPosition()].append(it.ptr());
  }
  foreach (const auto& pins, pinPositions) {
    if (pins.count() > 1) {
      msgs.append(std::make_shared<MsgOverlappingSymbolPins>(pins));
    }
  }
}

void SymbolCheck::checkMissingTexts(MsgList& msgs) const {
  QHash<QString, QVector<std::shared_ptr<const Text>>> texts;
  for (auto it = mSymbol.getTexts().begin(); it != mSymbol.getTexts().end();
       ++it) {
    texts[(*it).getText()].append(it.ptr());
  }
  if (texts.value("{{NAME}}").isEmpty()) {
    msgs.append(std::make_shared<MsgMissingSymbolName>());
  }
  if (texts.value("{{VALUE}}").isEmpty()) {
    msgs.append(std::make_shared<MsgMissingSymbolValue>());
  }
}

void SymbolCheck::checkWrongTextLayers(MsgList& msgs) const {
  QHash<QString, QString> textLayers = {
      std::make_pair("{{NAME}}", QString(GraphicsLayer::sSymbolNames)),
      std::make_pair("{{VALUE}}", QString(GraphicsLayer::sSymbolValues)),
  };
  for (auto it = mSymbol.getTexts().begin(); it != mSymbol.getTexts().end();
       ++it) {
    QString expectedLayer = textLayers.value((*it).getText());
    if ((!expectedLayer.isEmpty()) && ((*it).getLayerName() != expectedLayer)) {
      msgs.append(
          std::make_shared<MsgWrongSymbolTextLayer>(it.ptr(), expectedLayer));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
