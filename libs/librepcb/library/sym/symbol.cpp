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
    mPins(this),
    mPolygons(this),
    mCircles(this),
    mTexts(this),
    mRegisteredGraphicsItem(nullptr) {
}

Symbol::Symbol(const FilePath& elementDirectory, bool readOnly)
  : LibraryElement(elementDirectory, getShortElementName(),
                   getLongElementName(), readOnly),
    mPins(this),
    mPolygons(this),
    mCircles(this),
    mTexts(this),
    mRegisteredGraphicsItem(nullptr) {
  mPins.loadFromDomElement(mLoadingFileDocument);      // can throw
  mPolygons.loadFromDomElement(mLoadingFileDocument);  // can throw
  mCircles.loadFromDomElement(mLoadingFileDocument);   // can throw
  mTexts.loadFromDomElement(mLoadingFileDocument);     // can throw

  // backward compatibility, remove this some time!
  foreach (const SExpression& child,
           mLoadingFileDocument.getChildren("ellipse")) {
    mCircles.append(std::make_shared<Circle>(child));
  }

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

void Symbol::listObjectAdded(const SymbolPinList& list, int newIndex,
                             const std::shared_ptr<SymbolPin>& ptr) noexcept {
  Q_UNUSED(newIndex);
  Q_ASSERT(&list == &mPins);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addPin(*ptr);
}

void Symbol::listObjectAdded(const PolygonList& list, int newIndex,
                             const std::shared_ptr<Polygon>& ptr) noexcept {
  Q_UNUSED(newIndex);
  Q_ASSERT(&list == &mPolygons);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addPolygon(*ptr);
}

void Symbol::listObjectAdded(const CircleList& list, int newIndex,
                             const std::shared_ptr<Circle>& ptr) noexcept {
  Q_UNUSED(newIndex);
  Q_ASSERT(&list == &mCircles);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addCircle(*ptr);
}

void Symbol::listObjectAdded(const TextList& list, int newIndex,
                             const std::shared_ptr<Text>& ptr) noexcept {
  Q_UNUSED(newIndex);
  Q_ASSERT(&list == &mTexts);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->addText(*ptr);
}

void Symbol::listObjectRemoved(const SymbolPinList& list, int oldIndex,
                               const std::shared_ptr<SymbolPin>& ptr) noexcept {
  Q_UNUSED(oldIndex);
  Q_ASSERT(&list == &mPins);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removePin(*ptr);
}

void Symbol::listObjectRemoved(const PolygonList& list, int oldIndex,
                               const std::shared_ptr<Polygon>& ptr) noexcept {
  Q_UNUSED(oldIndex);
  Q_ASSERT(&list == &mPolygons);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removePolygon(*ptr);
}

void Symbol::listObjectRemoved(const CircleList& list, int oldIndex,
                               const std::shared_ptr<Circle>& ptr) noexcept {
  Q_UNUSED(oldIndex);
  Q_ASSERT(&list == &mCircles);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeCircle(*ptr);
}

void Symbol::listObjectRemoved(const TextList& list, int oldIndex,
                               const std::shared_ptr<Text>& ptr) noexcept {
  Q_UNUSED(oldIndex);
  Q_ASSERT(&list == &mTexts);
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->removeText(*ptr);
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
