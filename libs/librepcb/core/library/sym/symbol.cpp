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

#include "../../serialization/fileformatmigration.h"
#include "symbolcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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
    mPinsEditedSlot(*this, &Symbol::pinsEdited),
    mPolygonsEditedSlot(*this, &Symbol::polygonsEdited),
    mCirclesEditedSlot(*this, &Symbol::circlesEdited),
    mTextsEditedSlot(*this, &Symbol::textsEdited) {
  mPins.onEdited.attach(mPinsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mTexts.onEdited.attach(mTextsEditedSlot);
}

Symbol::Symbol(std::unique_ptr<TransactionalDirectory> directory,
               const SExpression& root)
  : LibraryElement(getShortElementName(), getLongElementName(), true,
                   std::move(directory), root),
    onEdited(*this),
    mPins(root),
    mPolygons(root),
    mCircles(root),
    mTexts(root),
    mPinsEditedSlot(*this, &Symbol::pinsEdited),
    mPolygonsEditedSlot(*this, &Symbol::polygonsEdited),
    mCirclesEditedSlot(*this, &Symbol::circlesEdited),
    mTextsEditedSlot(*this, &Symbol::textsEdited) {
  mPins.onEdited.attach(mPinsEditedSlot);
  mPolygons.onEdited.attach(mPolygonsEditedSlot);
  mCircles.onEdited.attach(mCirclesEditedSlot);
  mTexts.onEdited.attach(mTextsEditedSlot);
}

Symbol::~Symbol() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList Symbol::runChecks() const {
  SymbolCheck check(*this);
  return check.runChecks();  // can throw
}

std::unique_ptr<Symbol> Symbol::open(
    std::unique_ptr<TransactionalDirectory> directory) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  for (auto migration : FileFormatMigration::getMigrations(fileFormat)) {
    migration->upgradeSymbol(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const SExpression root = SExpression::parse(directory->read(fileName),
                                              directory->getAbsPath(fileName));
  return std::unique_ptr<Symbol>(new Symbol(std::move(directory), root));
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Symbol::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.ensureLineBreak();
  mPins.serialize(root);
  root.ensureLineBreak();
  mPolygons.serialize(root);
  root.ensureLineBreak();
  mCircles.serialize(root);
  root.ensureLineBreak();
  mTexts.serialize(root);
  root.ensureLineBreak();
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Symbol::pinsEdited(const SymbolPinList& list, int index,
                        const std::shared_ptr<const SymbolPin>& pin,
                        SymbolPinList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(pin);
  Q_UNUSED(event);
  onEdited.notify(Event::PinsEdited);
}

void Symbol::polygonsEdited(const PolygonList& list, int index,
                            const std::shared_ptr<const Polygon>& polygon,
                            PolygonList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(polygon);
  Q_UNUSED(event);
  onEdited.notify(Event::PolygonsEdited);
}

void Symbol::circlesEdited(const CircleList& list, int index,
                           const std::shared_ptr<const Circle>& circle,
                           CircleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(circle);
  Q_UNUSED(event);
  onEdited.notify(Event::CirclesEdited);
}

void Symbol::textsEdited(const TextList& list, int index,
                         const std::shared_ptr<const Text>& text,
                         TextList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(text);
  Q_UNUSED(event);
  onEdited.notify(Event::TextsEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
