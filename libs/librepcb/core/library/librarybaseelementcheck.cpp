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
#include "librarybaseelementcheck.h"

#include "librarybaseelement.h"
#include "librarybaseelementcheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryBaseElementCheck::LibraryBaseElementCheck(
    const LibraryBaseElement& element) noexcept
  : mElement(element) {
}

LibraryBaseElementCheck::~LibraryBaseElementCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList LibraryBaseElementCheck::runChecks() const {
  RuleCheckMessageList msgs;
  checkDefaultNameTitleCase(msgs);
  checkMissingAuthor(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void LibraryBaseElementCheck::checkDefaultNameTitleCase(MsgList& msgs) const {
  ElementName defaultName = mElement.getNames().getDefaultValue();
  if (!MsgNameNotTitleCase::isTitleCase(defaultName)) {
    msgs.append(std::make_shared<MsgNameNotTitleCase>(defaultName));
  }
}

void LibraryBaseElementCheck::checkMissingAuthor(MsgList& msgs) const {
  if (mElement.getAuthor().trimmed().isEmpty()) {
    msgs.append(std::make_shared<MsgMissingAuthor>());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
