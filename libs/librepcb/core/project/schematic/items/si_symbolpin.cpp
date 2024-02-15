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
#include "../../../utils/toolbox.h"
#include "../../../utils/transform.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/componentsignalinstance.h"
#include "../../circuit/netsignal.h"
#include "../schematic.h"
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
    onEdited(*this),
    mSymbol(symbol),
    mSymbolPin(nullptr),
    mPinSignalMapItem(nullptr),
    mComponentSignalInstance(nullptr),
    mOnSymbolEditedSlot(*this, &SI_SymbolPin::symbolEdited) {
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

  // Register to net signal changes.
  if (mComponentSignalInstance) {
    netSignalChanged(nullptr, mComponentSignalInstance->getNetSignal());
    connect(mComponentSignalInstance,
            &ComponentSignalInstance::netSignalChanged, this,
            &SI_SymbolPin::netSignalChanged);
    connect(mComponentSignalInstance, &ComponentSignalInstance::padNamesChanged,
            this, &SI_SymbolPin::updateNumbers);
  }

  updateTransform();
  updateName();
  updateNumbers();
  updateNumbersTransform();

  mSymbol.onEdited.attach(mOnSymbolEditedSlot);
}

SI_SymbolPin::~SI_SymbolPin() {
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& SI_SymbolPin::getLibPinUuid() const noexcept {
  return mSymbolPin->getUuid();
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
  SI_Base::addToSchematic();
}

void SI_SymbolPin::removeFromSchematic() {
  if ((!isAddedToSchematic()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentSignalInstance) {
    mComponentSignalInstance->unregisterSymbolPin(*this);  // can throw
  }
  SI_Base::removeFromSchematic();
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
  if (mRegisteredNetLines.count() <= 2) {
    onEdited.notify(Event::JunctionChanged);
  }
}

void SI_SymbolPin::unregisterNetLine(SI_NetLine& netline) {
  if ((!isAddedToSchematic()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  if (mRegisteredNetLines.count() <= 1) {
    onEdited.notify(Event::JunctionChanged);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SI_SymbolPin::symbolEdited(const SI_Symbol& obj,
                                SI_Symbol::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_Symbol::Event::PositionChanged:
    case SI_Symbol::Event::RotationChanged:
    case SI_Symbol::Event::MirroredChanged:
      updateTransform();
      break;
    default:
      qWarning() << "Unhandled switch-case in SI_SymbolPin::symbolEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SI_SymbolPin::netSignalChanged(NetSignal* from, NetSignal* to) noexcept {
  if ((!from) != (!to)) {
    onEdited.notify(Event::JunctionChanged);
  }

  if (mPinSignalMapItem) {
    netSignalNameChanged();
    if (from) {
      disconnect(from, &NetSignal::nameChanged, this,
                 &SI_SymbolPin::netSignalNameChanged);
    }
    if (to) {
      connect(to, &NetSignal::nameChanged, this,
              &SI_SymbolPin::netSignalNameChanged);
    }
  }
}

void SI_SymbolPin::netSignalNameChanged() noexcept {
  updateName();
  onEdited.notify(Event::NetNameChanged);
}

void SI_SymbolPin::updateTransform() noexcept {
  Transform transform(mSymbol);
  const Point position = transform.map(mSymbolPin->getPosition());
  const Angle rotation = transform.mapNonMirrorable(mSymbolPin->getRotation());
  if (position != mPosition) {
    mPosition = position;
    onEdited.notify(Event::PositionChanged);
    foreach (SI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
    }
  }
  if (rotation != mRotation) {
    mRotation = rotation;
    onEdited.notify(Event::RotationChanged);
    updateNumbersTransform();
  }
}

void SI_SymbolPin::updateName() noexcept {
  QString text;
  const CmpSigPinDisplayType displayType = mPinSignalMapItem->getDisplayType();
  if (displayType == CmpSigPinDisplayType::pinName()) {
    text = *mSymbolPin->getName();
  } else if (displayType == CmpSigPinDisplayType::componentSignal()) {
    if (mComponentSignalInstance) {
      text = *mComponentSignalInstance->getCompSignal().getName();
    }
  } else if (displayType == CmpSigPinDisplayType::netSignal()) {
    if (mComponentSignalInstance && mComponentSignalInstance->getNetSignal()) {
      text = *mComponentSignalInstance->getNetSignal()->getName();
    }
  } else {
    Q_ASSERT(displayType == CmpSigPinDisplayType::none());
  }

  if (text != mName) {
    mName = text;
    onEdited.notify(Event::NameChanged);
  }
}

void SI_SymbolPin::updateNumbers() noexcept {
  const QStringList numbers = mComponentSignalInstance
      ? mComponentSignalInstance->getPadNames()
      : QStringList();

  if (numbers != mNumbers) {
    mNumbers = numbers;
    mNumbersTruncated.clear();
    // Hide pin number if it's identical to the pin name shown on the
    // schematic. This avoids cluttering the schematic with redundant
    // information, mainly for components like connectors where pin names
    // and pad names are just numbers (1, 2, 3, ...). In such cases, hiding
    // pin numbers also reduces the risk of overlaps in case of non-standard
    // pin lengths (also often the case for connectors).
    const bool allowHide = ((mPinSignalMapItem->getDisplayType() ==
                             CmpSigPinDisplayType::pinName()) ||
                            (mPinSignalMapItem->getDisplayType() ==
                             CmpSigPinDisplayType::componentSignal()));
    if ((!allowHide) || (mNumbers.count() != 1) || (mNumbers.at(0) != mName)) {
      foreach (QString number, numbers) {
        if (!mNumbersTruncated.isEmpty()) {
          number.prepend(",");
        }
        if (mNumbersTruncated.length() + number.length() < 8) {
          mNumbersTruncated += number;
        } else {
          mNumbersTruncated += "…";
          break;
        }
      }
    }
    onEdited.notify(Event::NumbersChanged);
  }
}

void SI_SymbolPin::updateNumbersTransform() noexcept {
  const bool flip = Toolbox::isTextUpsideDown(mRotation);
  const Point position = mSymbolPin->getNumbersPosition(flip);
  const Alignment alignment = SymbolPin::getNumbersAlignment(flip);
  if (position != mNumbersPosition) {
    mNumbersPosition = position;
    onEdited.notify(Event::NumbersPositionChanged);
  }
  if (alignment != mNumbersAlignment) {
    mNumbersAlignment = alignment;
    onEdited.notify(Event::NumbersAlignmentChanged);
  }
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
