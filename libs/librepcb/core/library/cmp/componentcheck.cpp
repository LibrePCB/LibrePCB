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
#include "componentcheck.h"

#include "component.h"
#include "componentcheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentCheck::ComponentCheck(const Component& component) noexcept
  : LibraryElementCheck(component), mComponent(component) {
}

ComponentCheck::~ComponentCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList ComponentCheck::runChecks() const {
  RuleCheckMessageList msgs = LibraryElementCheck::runChecks();
  checkMissingPrefix(msgs);
  checkMissingDefaultValue(msgs);
  checkDuplicateSignalNames(msgs);
  checkSignalNamesInversionSign(msgs);
  checkSuspiciousForcedNets(msgs);
  checkMissingSymbolVariants(msgs);
  checkMissingSymbolVariantItems(msgs);
  checkNoPinsConnected(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void ComponentCheck::checkMissingPrefix(MsgList& msgs) const {
  if (mComponent.getPrefixes().getDefaultValue()->isEmpty()) {
    msgs.append(std::make_shared<MsgMissingComponentPrefix>());
  }
}

void ComponentCheck::checkMissingDefaultValue(MsgList& msgs) const {
  if (mComponent.getDefaultValue().trimmed().isEmpty()) {
    msgs.append(std::make_shared<MsgMissingComponentDefaultValue>());
  }
}

void ComponentCheck::checkDuplicateSignalNames(MsgList& msgs) const {
  QSet<CircuitIdentifier> names;
  for (const ComponentSignal& signal : mComponent.getSignals()) {
    if (names.contains(signal.getName())) {
      msgs.append(std::make_shared<MsgDuplicateSignalName>(signal));
    } else {
      names.insert(signal.getName());
    }
  }
}

void ComponentCheck::checkSignalNamesInversionSign(MsgList& msgs) const {
  for (auto it = mComponent.getSignals().begin();
       it != mComponent.getSignals().end(); ++it) {
    const QString name = *it->getName();
    if (name.startsWith("/") ||
        ((name.count() >= 2) && name.startsWith("n") && name.at(1).isUpper())) {
      msgs.append(
          std::make_shared<MsgNonFunctionalComponentSignalInversionSign>(
              it.ptr()));
    }
  }
}

void ComponentCheck::checkSuspiciousForcedNets(MsgList& msgs) const {
  // Do not emit "suspicious forced net" for single-signal components like
  // supply symbols (GND, VCC, ...).
  if (mComponent.getSignals().count() > 1) {
    for (auto it = mComponent.getSignals().begin();
         it != mComponent.getSignals().end(); ++it) {
      if (!it->getForcedNetName().isEmpty()) {
        msgs.append(std::make_shared<MsgSuspiciousForcedNets>());
        break;
      }
    }
  }
}

void ComponentCheck::checkMissingSymbolVariants(MsgList& msgs) const {
  if (mComponent.getSymbolVariants().isEmpty()) {
    msgs.append(std::make_shared<MsgMissingSymbolVariant>());
  }
}

void ComponentCheck::checkMissingSymbolVariantItems(MsgList& msgs) const {
  for (auto it = mComponent.getSymbolVariants().begin();
       it != mComponent.getSymbolVariants().end(); ++it) {
    std::shared_ptr<const ComponentSymbolVariant> symbVar = it.ptr();
    if (symbVar->getSymbolItems().isEmpty()) {
      msgs.append(std::make_shared<MsgMissingSymbolVariantItem>(symbVar));
    }
  }
}

void ComponentCheck::checkNoPinsConnected(MsgList& msgs) const {
  // This warning makes no sense if there are no component signals.
  if (mComponent.getSignals().isEmpty()) {
    return;
  }

  for (auto it = mComponent.getSymbolVariants().begin();
       it != mComponent.getSymbolVariants().end(); ++it) {
    std::shared_ptr<const ComponentSymbolVariant> symbVar = it.ptr();
    int connectedPins = 0;
    for (const ComponentSymbolVariantItem& item : symbVar->getSymbolItems()) {
      for (const ComponentPinSignalMapItem& map : item.getPinSignalMap()) {
        if (map.getSignalUuid()) {
          ++connectedPins;
        }
      }
    }
    if ((connectedPins == 0) && (!symbVar->getSymbolItems().isEmpty())) {
      msgs.append(std::make_shared<MsgNoPinsInSymbolVariantConnected>(symbVar));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
