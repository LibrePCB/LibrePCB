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
#include "electricalrulecheck.h"

#include "../../library/cmp/component.h"
#include "../../library/cmp/componentsignal.h"
#include "../circuit/circuit.h"
#include "../circuit/componentinstance.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netclass.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "../schematic/items/si_netpoint.h"
#include "../schematic/items/si_netsegment.h"
#include "../schematic/items/si_symbol.h"
#include "../schematic/items/si_symbolpin.h"
#include "../schematic/schematic.h"
#include "electricalrulecheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ElectricalRuleCheck::ElectricalRuleCheck(const Project& project) noexcept
  : mProject(project) {
}

ElectricalRuleCheck::~ElectricalRuleCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList ElectricalRuleCheck::runChecks() const {
  mOpenNetSignals.clear();

  RuleCheckMessageList msgs;
  checkNetClasses(msgs);
  checkNetSignals(msgs);
  checkComponents(msgs);
  checkSchematics(msgs);
  return msgs;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ElectricalRuleCheck::checkNetClasses(RuleCheckMessageList& msgs) const {
  // Don't warn if there's only one netclass, as we need one to be used as
  // default when adding a new wire.
  if (mProject.getCircuit().getNetClasses().count() <= 1) {
    return;
  }

  foreach (const NetClass* netClass, mProject.getCircuit().getNetClasses()) {
    if (!netClass->isUsed()) {
      msgs.append(std::make_shared<ErcMsgUnusedNetClass>(*netClass));
    }
  }
}

void ElectricalRuleCheck::checkNetSignals(RuleCheckMessageList& msgs) const {
  foreach (const NetSignal* net, mProject.getCircuit().getNetSignals()) {
    // Raise a warning if the net signal is connected to less then two component
    // signals. But do not count component signals of schematic-only components
    // since these are just "virtual" connections, i.e. not represented by a
    // real pad (see https://github.com/LibrePCB/LibrePCB/issues/739).
    const QList<ComponentSignalInstance*>& sigs = net->getComponentSignals();
    int registeredRealComponentCount = sigs.count();
    if (registeredRealComponentCount >= 2) {  // Optimization
      foreach (const ComponentSignalInstance* sig, sigs) {
        if (sig->getComponentInstance().getLibComponent().isSchematicOnly()) {
          --registeredRealComponentCount;
        }
      }
    }
    if (registeredRealComponentCount < 2) {
      mOpenNetSignals.insert(net);
      msgs.append(std::make_shared<ErcMsgOpenNet>(*net));
    }
  }
}

void ElectricalRuleCheck::checkComponents(RuleCheckMessageList& msgs) const {
  foreach (const ComponentInstance* cmp,
           mProject.getCircuit().getComponentInstances()) {
    checkComponentSignals(*cmp, msgs);

    // Check for unplaced gates.
    for (const ComponentSymbolVariantItem& gate :
         cmp->getSymbolVariant().getSymbolItems()) {
      if (!cmp->getSymbols().contains(gate.getUuid())) {
        if (gate.isRequired()) {
          msgs.append(std::make_shared<ErcMsgUnplacedRequiredGate>(*cmp, gate));
        } else {
          msgs.append(std::make_shared<ErcMsgUnplacedOptionalGate>(*cmp, gate));
        }
      }
    }
  }
}

void ElectricalRuleCheck::checkComponentSignals(
    const ComponentInstance& cmp, RuleCheckMessageList& msgs) const {
  foreach (const ComponentSignalInstance* sig, cmp.getSignals()) {
    // Check for forced net name conflict.
    const QString netName =
        sig->getNetSignal() ? (*sig->getNetSignal()->getName()) : QString();
    if ((sig->getCompSignal().isRequired()) && (!sig->getNetSignal())) {
      msgs.append(std::make_shared<ErcMsgUnconnectedRequiredSignal>(*sig));
    } else if (sig->isNetSignalNameForced() &&
               (sig->getForcedNetSignalName() != netName)) {
      msgs.append(std::make_shared<ErcMsgForcedNetSignalNameConflict>(*sig));
    }
  }
}

void ElectricalRuleCheck::checkSchematics(RuleCheckMessageList& msgs) const {
  foreach (const Schematic* schematic, mProject.getSchematics()) {
    checkSymbols(*schematic, msgs);
    checkNetSegments(*schematic, msgs);
  }
}

void ElectricalRuleCheck::checkSymbols(const Schematic& schematic,
                                       RuleCheckMessageList& msgs) const {
  foreach (const SI_Symbol* symbol, schematic.getSymbols()) {
    checkPins(*symbol, msgs);
  }
}

void ElectricalRuleCheck::checkPins(const SI_Symbol& symbol,
                                    RuleCheckMessageList& msgs) const {
  foreach (const SI_SymbolPin* pin, symbol.getPins()) {
    if ((pin->getNetLines().isEmpty()) && (pin->getCompSigInstNetSignal())) {
      msgs.append(std::make_shared<ErcMsgConnectedPinWithoutWire>(*pin));
    }
  }
}

void ElectricalRuleCheck::checkNetSegments(const Schematic& schematic,
                                           RuleCheckMessageList& msgs) const {
  foreach (const SI_NetSegment* netSegment, schematic.getNetSegments()) {
    checkNetPoints(*netSegment, msgs);

    // If there are no net labels, check for any open wire. But only if there's
    // no "open net" warning on the net raised, since this would be quite a
    // duplicate warning.
    if (netSegment->getNetLabels().isEmpty() &&
        (!mOpenNetSignals.contains(&netSegment->getNetSignal()))) {
      foreach (const SI_NetLine* netLine, netSegment->getNetLines()) {
        if (netLine->getStartPoint().isOpen() ||
            netLine->getEndPoint().isOpen()) {
          msgs.append(std::make_shared<ErcMsgOpenWireInSegment>(*netSegment));
          break;
        }
      }
    }
  }
}

void ElectricalRuleCheck::checkNetPoints(const SI_NetSegment& netSegment,
                                         RuleCheckMessageList& msgs) const {
  foreach (const SI_NetPoint* netPoint, netSegment.getNetPoints()) {
    if (netPoint->getNetLines().isEmpty()) {
      msgs.append(std::make_shared<ErcMsgUnconnectedJunction>(*netPoint));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
