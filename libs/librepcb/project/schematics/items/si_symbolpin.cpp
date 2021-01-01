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

#include "../../circuit/componentinstance.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../../erc/ercmsg.h"
#include "si_symbol.h"

#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolpin.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

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
  if (cmpSignalUuid)
    mComponentSignalInstance =
        mSymbol.getComponentInstance().getSignalInstance(*cmpSignalUuid);

  mGraphicsItem.reset(new SGI_SymbolPin(*this));
  updatePosition();

  // create ERC messages
  mErcMsgUnconnectedRequiredPin.reset(new ErcMsg(
      mSchematic.getProject(), *this,
      QString("%1/%2")
          .arg(mSymbol.getUuid().toStr())
          .arg(mSymbolPin->getUuid().toStr()),
      "UnconnectedRequiredPin", ErcMsg::ErcMsgType_t::SchematicError));
  updateErcMessages();
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
  library::CmpSigPinDisplayType displayType =
      mPinSignalMapItem->getDisplayType();
  if (displayType == library::CmpSigPinDisplayType::pinName()) {
    text = *mSymbolPin->getName();
  } else if (displayType == library::CmpSigPinDisplayType::componentSignal()) {
    if (mComponentSignalInstance) {
      text = *mComponentSignalInstance->getCompSignal().getName();
    }
  } else if (displayType == library::CmpSigPinDisplayType::netSignal()) {
    if (mComponentSignalInstance) {
      if (mComponentSignalInstance->getNetSignal()) {
        text = *mComponentSignalInstance->getNetSignal()->getName();
      }
    }
  } else if (displayType != library::CmpSigPinDisplayType::none()) {
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
  }
  if (getCompSigInstNetSignal()) {
    mHighlightChangedConnection =
        connect(getCompSigInstNetSignal(), &NetSignal::highlightedChanged,
                [this]() { mGraphicsItem->update(); });
  }
  SI_Base::addToSchematic(mGraphicsItem.data());
  updateErcMessages();
  mGraphicsItem->updateCacheAndRepaint();
}

void SI_SymbolPin::removeFromSchematic() {
  if ((!isAddedToSchematic()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->unregisterSymbolPin(*this);  // can throw
  }
  if (getCompSigInstNetSignal()) {
    disconnect(mHighlightChangedConnection);
  }
  SI_Base::removeFromSchematic(mGraphicsItem.data());
  updateErcMessages();
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
  updateErcMessages();
  mGraphicsItem
      ->updateCacheAndRepaint();  // re-check whether to fill the circle or not
}

void SI_SymbolPin::unregisterNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  netline.updateLine();
  updateErcMessages();
  mGraphicsItem
      ->updateCacheAndRepaint();  // re-check whether to fill the circle or not
}

void SI_SymbolPin::updatePosition() noexcept {
  mPosition = mSymbol.mapToScene(mSymbolPin->getPosition());
  mRotation = mSymbol.getRotation() + mSymbolPin->getRotation();
  mGraphicsItem->setPos(mPosition.toPxQPointF());
  updateGraphicsItemTransform();
  mGraphicsItem->updateCacheAndRepaint();
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
  mGraphicsItem->update();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void SI_SymbolPin::updateErcMessages() noexcept {
  mErcMsgUnconnectedRequiredPin->setMsg(
      tr("Unconnected pin: \"%1\" of symbol \"%2\"")
          .arg(getDisplayText(true, true))
          .arg(mSymbol.getName()));

  mErcMsgUnconnectedRequiredPin->setVisible(isAddedToSchematic() &&
                                            isRequired() && (!isUsed()));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SI_SymbolPin::updateGraphicsItemTransform() noexcept {
  QTransform t;
  if (mSymbol.getMirrored()) t.scale(qreal(-1), qreal(1));
  t.rotate(-mRotation.toDeg());
  mGraphicsItem->setTransform(t);
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

}  // namespace project
}  // namespace librepcb
