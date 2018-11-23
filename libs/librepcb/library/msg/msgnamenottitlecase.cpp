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
#include "msgnamenottitlecase.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgNameNotTitleCase::MsgNameNotTitleCase(const ElementName& name) noexcept
  : LibraryElementCheckMessage(
        Severity::Hint, QString(tr("Name not title case: '%1'")).arg(*name),
        tr("Generally the library element name should be written in title case "
           "(for consistency). As the current name has words starting with a "
           "lowercase character, it seems that it is not title cases. If this "
           "assumption is wrong, just ignore this message.")),
    mName(name) {
}

MsgNameNotTitleCase::~MsgNameNotTitleCase() noexcept {
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool MsgNameNotTitleCase::isTitleCase(const ElementName& name) noexcept {
  bool lastCharWasSpace = true;
  foreach (const QChar& c, *name) {
    if (lastCharWasSpace && c.isLetter() && c.isLower()) {
      return false;
    }
    lastCharWasSpace = c.isSpace();
  }
  return true;
}

ElementName MsgNameNotTitleCase::getFixedName(
    const ElementName& name) noexcept {
  QString newName;
  bool    lastCharWasSpace = true;
  foreach (QChar c, *name) {
    if (lastCharWasSpace && c.isLetter() && c.isLower()) {
      c = c.toUpper();
    }
    newName.append(c);
    lastCharWasSpace = c.isSpace();
  }
  try {
    return ElementName(newName);  // Could throw, but should really not!
  } catch (const Exception& e) {
    qCritical() << "Could not fixup invalid name:" << e.getMsg();
    return name;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
