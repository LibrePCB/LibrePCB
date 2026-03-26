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
#include "librarycategorycheck.h"

#include "librarycategory.h"
#include "librarycategorycheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryCategoryCheck::LibraryCategoryCheck(
    const LibraryCategory& category) noexcept
  : LibraryBaseElementCheck(category), mCategory(category) {
}

LibraryCategoryCheck::~LibraryCategoryCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList LibraryCategoryCheck::runChecks() const {
  RuleCheckMessageList msgs = LibraryBaseElementCheck::runChecks();
  checkParent(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void LibraryCategoryCheck::checkParent(MsgList& msgs) const {
  if (mCategory.getParentUuid() == mCategory.getUuid()) {
    msgs.append(std::make_shared<MsgInvalidParent>());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
