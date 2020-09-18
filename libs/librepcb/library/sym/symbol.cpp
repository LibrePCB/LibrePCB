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
#include "symbol.h"

#include "symbolcheck.h"
#include "symbolgraphicsitem.h"

#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Symbol::Symbol(const Uuid& uuid, const Version& version, const QString& author,
               const ElementName& name_en_US, const QString& description_en_US,
               const QString& keywords_en_US)
  : LibraryElement(getShortElementName(), getLongElementName(), uuid, version,
                   author, name_en_US, description_en_US, keywords_en_US),
    onEdited(*this),
    mPins(),
    mPolygons(),
    mCircles(),
    mTexts(),
    mRegisteredGraphicsItem(nullptr),
    mPinsEditedSlot(*this, &Symbol::pinsEdited),
    mPolygonsEditedSlot(*this, &Symbol::polygonsEdited),
    mCirclesEditedSlot(*this, &Symbol::circlesEdited),
    mTextsEditedSlot(*this, &Symbol::textsEdited) {
  mPins.onEdited.attach(mPinsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mTexts.onEdited.attach(mTextsEditedSlot);
}

Symbol::Symbol(std::unique_ptr<TransactionalDirectory> directory)
  : LibraryElement(std::move(directory), getShortElementName(),
                   getLongElementName()),
    onEdited(*this),
    mPins(mLoadingFileDocument),
    mPolygons(mLoadingFileDocument),
    mCircles(mLoadingFileDocument),
    mTexts(mLoadingFileDocument),
    mRegisteredGraphicsItem(nullptr),
    mPinsEditedSlot(*this, &Symbol::pinsEdited),
    mPolygonsEditedSlot(*this, &Symbol::polygonsEdited),
    mCirclesEditedSlot(*this, &Symbol::circlesEdited),
    mTextsEditedSlot(*this, &Symbol::textsEdited) {
  mPins.onEdited.attach(mPinsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mTexts.onEdited.attach(mTextsEditedSlot);
  cleanupAfterLoadingElementFromFile();
}

Symbol::~Symbol() noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList Symbol::runChecks() const {
  SymbolCheck check(*this);
  return check.runChecks();  // can throw
}

void Symbol::registerGraphicsItem(SymbolGraphicsItem& item) noexcept {
  Q_ASSERT(!mRegisteredGraphicsItem);
  mRegisteredGraphicsItem = &item;
}

void Symbol::unregisterGraphicsItem(SymbolGraphicsItem& item) noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == &item);
  mRegisteredGraphicsItem = nullptr;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Symbol::pinsEdited(const SymbolPinList& list, int index,
                        const std::shared_ptr<const SymbolPin>& pin,
                        SymbolPinList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case SymbolPinList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addPin(const_cast<SymbolPin&>(*pin));
      }
      break;
    case SymbolPinList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removePin(const_cast<SymbolPin&>(*pin));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::PinsEdited);
}

void Symbol::polygonsEdited(const PolygonList& list, int index,
                            const std::shared_ptr<const Polygon>& polygon,
                            PolygonList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case PolygonList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addPolygon(const_cast<Polygon&>(*polygon));
      }
      break;
    case PolygonList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removePolygon(const_cast<Polygon&>(*polygon));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::PolygonsEdited);
}

void Symbol::circlesEdited(const CircleList& list, int index,
                           const std::shared_ptr<const Circle>& circle,
                           CircleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case CircleList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addCircle(const_cast<Circle&>(*circle));
      }
      break;
    case CircleList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removeCircle(const_cast<Circle&>(*circle));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::CirclesEdited);
}

void Symbol::textsEdited(const TextList& list, int index,
                         const std::shared_ptr<const Text>& text,
                         TextList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  switch (event) {
    case TextList::Event::ElementAdded:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->addText(const_cast<Text&>(*text));
      }
      break;
    case TextList::Event::ElementRemoved:
      if (mRegisteredGraphicsItem) {
        mRegisteredGraphicsItem->removeText(const_cast<Text&>(*text));
      }
      break;
    default:
      break;
  }
  onEdited.notify(Event::TextsEdited);
}

void Symbol::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  mPins.serialize(root);
  mPolygons.serialize(root);
  mCircles.serialize(root);
  mTexts.serialize(root);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
