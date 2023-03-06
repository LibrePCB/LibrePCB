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
#include "si_symbolpin.h"

#include "../../../library/cmp/component.h"
#include "../../../library/sym/symbol.h"
#include "../../../library/sym/symbolpin.h"
#include "../../../utils/transform.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "si_symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_SymbolPin::SI_SymbolPin(SI_Symbol& symbol, const Uuid& pinUuid)
  : SI_Base(symbol.getSchematic()),
    mSymbol(symbol),
    mSymbolPin(nullptr),
    mPinSignalMapItem(nullptr),
    mComponentSignalInstance(nullptr) {
  // read attributes
  mSymbolPin =
      mSymbol.getLibSymbol().getPins().get(pinUuid).get();  // can throw
  mPinSignalMapItem = mSymbol.getCompSymbVarItem()
                          .getPinSignalMap()
                          .get(pinUuid)
                          .get();  // can throw
  tl::optional<Uuid> cmpSignalUuid = mPinSignalMapItem->getSignalUuid();
  if (cmpSignalUuid) {
    mComponentSignalInstance =
        mSymbol.getComponentInstance().getSignalInstance(*cmpSignalUuid);
  }

  mGraphicsItem.reset(new SGI_SymbolPin(*this));
  updatePosition(true);
}

SI_SymbolPin::~SI_SymbolPin() {
  Q_ASSERT(!isUsed());
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& SI_SymbolPin::getLibPinUuid() const noexcept {
  return mSymbolPin->getUuid();
}

QString SI_SymbolPin::getDisplayText(bool returnCmpSignalNameIfEmpty,
                                     bool returnPinNameIfEmpty) const noexcept {
  QString text;
  CmpSigPinDisplayType displayType = mPinSignalMapItem->getDisplayType();
  if (displayType == CmpSigPinDisplayType::pinName()) {
    text = *mSymbolPin->getName();
  } else if (displayType == CmpSigPinDisplayType::componentSignal()) {
    if (mComponentSignalInstance) {
      text = *mComponentSignalInstance->getCompSignal().getName();
    }
  } else if (displayType == CmpSigPinDisplayType::netSignal()) {
    if (mComponentSignalInstance) {
      if (mComponentSignalInstance->getNetSignal()) {
        text = *mComponentSignalInstance->getNetSignal()->getName();
      }
    }
  } else if (displayType != CmpSigPinDisplayType::none()) {
    Q_ASSERT(false);
  }
  if (text.isEmpty() && returnCmpSignalNameIfEmpty && mComponentSignalInstance)
    text = *mComponentSignalInstance->getCompSignal().getName();
  if (text.isEmpty() && returnPinNameIfEmpty) text = *mSymbolPin->getName();
  return text;
}

NetSignal* SI_SymbolPin::getCompSigInstNetSignal() const noexcept {
  if (mComponentSignalInstance) {
    return mComponentSignalInstance->getNetSignal();
  } else {
    return nullptr;
  }
}

SI_NetSegment* SI_SymbolPin::getNetSegmentOfLines() const noexcept {
  auto it = mRegisteredNetLines.constBegin();
  return (it != mRegisteredNetLines.constEnd()) ? &((*it)->getNetSegment())
                                                : nullptr;
}

bool SI_SymbolPin::isRequired() const noexcept {
  if (mComponentSignalInstance) {
    return mComponentSignalInstance->getCompSignal().isRequired();
  } else {
    return false;
  }
}

bool SI_SymbolPin::isVisibleJunction() const noexcept {
  return (mRegisteredNetLines.count() > 1);
}

NetLineAnchor SI_SymbolPin::toNetLineAnchor() const noexcept {
  return NetLineAnchor::pin(mSymbol.getUuid(), mSymbolPin->getUuid());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_SymbolPin::addToSchematic() {
  if (isAddedToSchematic() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->registerSymbolPin(*this);  // can throw
    mNetSignalChangedConnection = connect(
        mComponentSignalInstance, &ComponentSignalInstance::netSignalChanged,
        this, &SI_SymbolPin::netSignalChanged);
  }
  if (NetSignal* netsignal = getCompSigInstNetSignal()) {
    mNetSignalRenamedConnection =
        connect(netsignal, &NetSignal::nameChanged, this,
                [this]() { mGraphicsItem->updateData(); });
    mHighlightChangedConnection =
        connect(netsignal, &NetSignal::highlightedChanged, this,
                [this]() { mGraphicsItem->updateSelection(); });
  }
  SI_Base::addToSchematic(mGraphicsItem.data());
  mGraphicsItem->updateData();
  mGraphicsItem->updateSelection();
}

void SI_SymbolPin::removeFromSchematic() {
  if ((!isAddedToSchematic()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->unregisterSymbolPin(*this);  // can throw
    disconnect(mNetSignalChangedConnection);
  }
  if (getCompSigInstNetSignal()) {
    disconnect(mNetSignalRenamedConnection);
    disconnect(mHighlightChangedConnection);
  }
  SI_Base::removeFromSchematic(mGraphicsItem.data());
}

void SI_SymbolPin::registerNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (mRegisteredNetLines.contains(&netline)) ||
      (netline.getSchematic() != mSchematic)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (&netline.getNetSignalOfNetSegment() != getCompSigInstNetSignal()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Line of net \"%1\" is not allowed to be connected to "
                "pin \"%2\" of component \"%3\" (%4) since it is connected "
                "to the net \"%5\".")
            .arg(*netline.getNetSignalOfNetSegment().getName(),
                 getComponentSignalNameOrPinUuid(),
                 *mSymbol.getComponentInstance().getName(),
                 getLibraryComponentName(), getNetSignalName()));
  }
  foreach (const SI_NetLine* l, mRegisteredNetLines) {
    if (&l->getNetSegment() != &netline.getNetSegment()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("There are lines from multiple net segments connected to "
                  "the pin \"%1\" of component \"%2\" (%3).")
              .arg(getComponentSignalNameOrPinUuid(),
                   *mSymbol.getComponentInstance().getName(),
                   getLibraryComponentName()));
    }
  }
  mRegisteredNetLines.insert(&netline);
  netline.updateLine();
  mGraphicsItem->updateData();
}

void SI_SymbolPin::unregisterNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  netline.updateLine();
  mGraphicsItem->updateData();
}

void SI_SymbolPin::updatePosition(bool mirroredOrRotated) noexcept {
  Transform transform(mSymbol);
  mPosition = transform.map(mSymbolPin->getPosition());
  mRotation = transform.map(mSymbolPin->getRotation());
  mGraphicsItem->setPos(mPosition.toPxQPointF());
  if (mirroredOrRotated) {
    mGraphicsItem->updateTransform();
  }
  foreach (SI_NetLine* netline, mRegisteredNetLines) { netline->updateLine(); }
}

/*******************************************************************************
 *  Inherited from SI_Base
 ******************************************************************************/

QPainterPath SI_SymbolPin::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

void SI_SymbolPin::setSelected(bool selected) noexcept {
  SI_Base::setSelected(selected);
  mGraphicsItem->updateSelection();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SI_SymbolPin::netSignalChanged(NetSignal* from, NetSignal* to) noexcept {
  if (from) {
    disconnect(mNetSignalRenamedConnection);
    disconnect(mHighlightChangedConnection);
  }
  if (to) {
    mNetSignalRenamedConnection =
        connect(to, &NetSignal::nameChanged, this,
                [this]() { mGraphicsItem->updateData(); });
    mHighlightChangedConnection =
        connect(to, &NetSignal::highlightedChanged, this,
                [this]() { mGraphicsItem->updateSelection(); });
  }
  mGraphicsItem->updateData();
  mGraphicsItem->updateSelection();
}

QString SI_SymbolPin::getLibraryComponentName() const noexcept {
  return *mSymbol.getComponentInstance()
              .getLibComponent()
              .getNames()
              .getDefaultValue();
}

QString SI_SymbolPin::getComponentSignalNameOrPinUuid() const noexcept {
  return mComponentSignalInstance
      ? *mComponentSignalInstance->getCompSignal().getName()
      : mSymbolPin->getUuid().toStr();
}

QString SI_SymbolPin::getNetSignalName() const noexcept {
  if (const NetSignal* signal = getCompSigInstNetSignal()) {
    return *signal->getName();
  } else {
    return QString();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
