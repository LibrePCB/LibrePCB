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
#include "electricalrulecheckmessages.h"

#include "../../library/cmp/componentsignal.h"
#include "../../library/cmp/componentsymbolvariantitem.h"
#include "../../library/sym/symbolpin.h"
#include "../circuit/componentinstance.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netclass.h"
#include "../circuit/netsignal.h"
#include "../schematic/items/si_netpoint.h"
#include "../schematic/items/si_netsegment.h"
#include "../schematic/items/si_symbol.h"
#include "../schematic/items/si_symbolpin.h"
#include "../schematic/schematic.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  ErcMsgBase
 ******************************************************************************/

bool ErcMsgBase::setLocation(const NetSignal& net) noexcept {
  for (const SI_NetSegment* seg : net.getSchematicNetSegments()) {
    if (setLocation(*seg)) {
      return true;
    }
  }
  for (const ComponentSignalInstance* cmpSig : net.getComponentSignals()) {
    if (setLocation(*cmpSig)) {
      return true;
    }
  }
  return false;
}

bool ErcMsgBase::setLocation(const ComponentInstance& component) noexcept {
  for (const SI_Symbol* sym : component.getSymbols()) {
    setLocation(*sym);
    return true;
  }
  return false;
}

bool ErcMsgBase::setLocation(const ComponentSignalInstance& signal) noexcept {
  for (const SI_SymbolPin* pin : signal.getRegisteredSymbolPins()) {
    setLocation(*pin);
    return true;
  }
  return setLocation(signal.getComponentInstance());
}

void ErcMsgBase::setLocation(const SI_Symbol& symbol) noexcept {
  mSchematic = symbol.getSchematic().getUuid();
  mLocations.append(
      Path::circle(PositiveLength(2000000)).translated(symbol.getPosition()));
}

void ErcMsgBase::setLocation(const SI_SymbolPin& pin) noexcept {
  mLocations.append(
      Path::circle(PositiveLength(1100000)).translated(pin.getPosition()));
  mSchematic = pin.getSchematic().getUuid();
}

bool ErcMsgBase::setLocation(const SI_NetSegment& segment) noexcept {
  for (const SI_NetLine* nl : segment.getNetLines()) {
    mLocations.append(Path::obround(nl->getP1().getPosition(),
                                    nl->getP2().getPosition(),
                                    PositiveLength(nl->getWidth() + 1)));
  }
  if (!segment.getNetLines().isEmpty()) {
    mSchematic = segment.getSchematic().getUuid();
    return true;
  }
  return false;
}

void ErcMsgBase::setLocation(const SI_NetPoint& netPoint) noexcept {
  mSchematic = netPoint.getSchematic().getUuid();
  mLocations.append(
      Path::circle(PositiveLength(1100000)).translated(netPoint.getPosition()));
}

void ErcMsgBase::setLocation(const SI_NetLine& netLine) noexcept {
  mSchematic = netLine.getSchematic().getUuid();
  mLocations.append(Path::obround(netLine.getP1().getPosition(),
                                  netLine.getP2().getPosition(),
                                  PositiveLength(netLine.getWidth() + 1)));
}

/*******************************************************************************
 *  ErcMsgUnusedNetClass
 ******************************************************************************/

ErcMsgUnusedNetClass::ErcMsgUnusedNetClass(const NetClass& netClass) noexcept
  : ErcMsgBase(Severity::Hint,
               tr("Unused net class: '%1'").arg(*netClass.getName()),
               tr("There are no nets assigned to the net class, so you "
                  "could remove it."),
               "unused_netclass") {
  mApproval->appendChild("netclass", netClass.getUuid());
}

/*******************************************************************************
 *  ErcMsgOpenNet
 ******************************************************************************/

ErcMsgOpenNet::ErcMsgOpenNet(const NetSignal& net) noexcept
  : ErcMsgBase(Severity::Warning,
               tr("Less than two pins in net: '%1'").arg(*net.getName()),
               tr("The net is connected to less than two pins, so it "
                  "does not represent an electrical connection. Check if "
                  "you missed to connect more pins."),
               "open_net") {
  mApproval->appendChild("net", net.getUuid());

  setLocation(net);
}

/*******************************************************************************
 *  ErcMsgOpenWireInSegment
 ******************************************************************************/

ErcMsgOpenWireInSegment::ErcMsgOpenWireInSegment(
    const SI_NetSegment& segment, const SI_NetLine& openWire) noexcept
  : ErcMsgBase(
        Severity::Warning,
        tr("Open wire in net: '%1'").arg(*segment.getNetSignal().getName()),
        tr("The wire has an open (unconnected) end with no net "
           "label attached, thus is looks like a mistake. Check "
           "if a connection to another wire or pin is missing (denoted by a "
           "cross mark)."),
        "open_wire") {
  mApproval->appendChild("segment", segment.getUuid());

  setLocation(openWire);
}

/*******************************************************************************
 *  ErcMsgUnconnectedRequiredSignal
 ******************************************************************************/

ErcMsgUnconnectedRequiredSignal::ErcMsgUnconnectedRequiredSignal(
    const ComponentSignalInstance& signal) noexcept
  : ErcMsgBase(Severity::Error,
               tr("Unconnected component signal: '%1:%2'")
                   .arg(*signal.getComponentInstance().getName(),
                        *signal.getCompSignal().getName()),
               tr("The component signal is marked as required, but is "
                  "not connected to any net. Add a wire to the "
                  "corresponding symbol pin to connect it to a net."),
               "unconnected_required_signal") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("component", signal.getComponentInstance().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("signal", signal.getCompSignal().getUuid());
  mApproval->ensureLineBreak();

  setLocation(signal);
}

/*******************************************************************************
 *  ErcMsgForcedNetSignalNameConflict
 ******************************************************************************/

ErcMsgForcedNetSignalNameConflict::ErcMsgForcedNetSignalNameConflict(
    const ComponentSignalInstance& signal) noexcept
  : ErcMsgBase(
        Severity::Error,
        tr("Net name conflict: '%1' != '%2' ('%3:%4')")
            .arg(getSignalNet(signal), signal.getForcedNetSignalName(),
                 *signal.getComponentInstance().getName(),
                 *signal.getCompSignal().getName()),
        tr("The component signal requires the attached net to be named '%1', "
           "but it is named '%2'. Either rename the net manually or remove "
           "this connection.")
            .arg(signal.getForcedNetSignalName(), getSignalNet(signal)),
        "forced_net_name_conflict") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("component", signal.getComponentInstance().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("signal", signal.getCompSignal().getUuid());
  mApproval->ensureLineBreak();

  setLocation(signal);
}

QString ErcMsgForcedNetSignalNameConflict::getSignalNet(
    const ComponentSignalInstance& signal) noexcept {
  return (signal.getNetSignal() ? (*signal.getNetSignal()->getName())
                                : QString());
}

/*******************************************************************************
 *  ErcMsgUnplacedRequiredGate
 ******************************************************************************/

ErcMsgUnplacedRequiredGate::ErcMsgUnplacedRequiredGate(
    const ComponentInstance& component,
    const ComponentSymbolVariantItem& gate) noexcept
  : ErcMsgBase(Severity::Error,
               tr("Unplaced required gate: '%1:%2'")
                   .arg(*component.getName(), *gate.getSuffix()),
               tr("The gate '%1' of '%2' is marked as required, but it "
                  "is not added to the schematic.")
                   .arg(*gate.getSuffix(), *component.getName()),
               "unplaced_required_gate") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("component", component.getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("gate", gate.getUuid());
  mApproval->ensureLineBreak();

  setLocation(component);
}

/*******************************************************************************
 *  ErcMsgUnplacedOptionalGate
 ******************************************************************************/

ErcMsgUnplacedOptionalGate::ErcMsgUnplacedOptionalGate(
    const ComponentInstance& component,
    const ComponentSymbolVariantItem& gate) noexcept
  : ErcMsgBase(
        Severity::Warning,
        tr("Unplaced gate: '%1:%2'")
            .arg(*component.getName(), *gate.getSuffix()),
        tr("The optional gate '%1' of '%2' is not added to the schematic.")
            .arg(*gate.getSuffix(), *component.getName()),
        "unplaced_optional_gate") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("component", component.getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("gate", gate.getUuid());
  mApproval->ensureLineBreak();

  setLocation(component);
}

/*******************************************************************************
 *  ErcMsgConnectedPinWithoutWire
 ******************************************************************************/

ErcMsgConnectedPinWithoutWire::ErcMsgConnectedPinWithoutWire(
    const SI_SymbolPin& pin) noexcept
  : ErcMsgBase(
        Severity::Warning,
        tr("Connected pin without wire: '%1:%2'")
            .arg(pin.getSymbol().getName(), pin.getName()),
        tr("The pin is electrically connected to a net, but has no wire "
           "attached so this connection is not visible in the schematic. Add a "
           "wire to make the connection visible."),
        "connected_pin_without_wire") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("schematic", pin.getSchematic().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("symbol", pin.getSymbol().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("pin", pin.getLibPinUuid());
  mApproval->ensureLineBreak();

  setLocation(pin);
}

/*******************************************************************************
 *  ErcMsgUnconnectedJunction
 ******************************************************************************/

ErcMsgUnconnectedJunction::ErcMsgUnconnectedJunction(
    const SI_NetPoint& netPoint) noexcept
  : ErcMsgBase(
        Severity::Hint,
        tr("Unconnected junction in net: '%1'")
            .arg(*netPoint.getNetSignalOfNetSegment().getName()),
        "There's an invisible junction in the schematic without any wire "
        "attached. This should not happen, please report it as a bug. But "
        "no worries, this issue is not harmful at all so you can safely "
        "ignore this message.",
        "unconnected_junction") {
  mApproval->ensureLineBreak();
  mApproval->appendChild("schematic", netPoint.getSchematic().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("netsegment", netPoint.getNetSegment().getUuid());
  mApproval->ensureLineBreak();
  mApproval->appendChild("junction", netPoint.getUuid());
  mApproval->ensureLineBreak();

  setLocation(netPoint);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
