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
#include "gerberattributewriter.h"

#include "gerberattribute.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GerberAttributeWriter::GerberAttributeWriter() noexcept : mDictionary() {
}

GerberAttributeWriter::~GerberAttributeWriter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QString GerberAttributeWriter::setAttributes(
    const QList<GerberAttribute>& attributes) noexcept {
  // Check which attributes need to be deleted or set.
  QList<GerberAttribute> toDelete;
  QList<GerberAttribute> toSet = attributes;
  foreach (const GerberAttribute& currentAttribute, mDictionary) {
    bool isStillSet = false;
    foreach (const GerberAttribute& newAttribute, attributes) {
      if (newAttribute.getKey() == currentAttribute.getKey()) {
        isStillSet = true;
        if (newAttribute == currentAttribute) {
          toSet.removeOne(newAttribute);
        }
      }
    }
    if (!isStillSet) {
      toDelete.append(GerberAttribute::unset(currentAttribute.getKey()));
    }
  }

  // If all attributes were removed, use the delete-all command.
  if ((toDelete.count() > 0) && (toDelete.count() == mDictionary.count())) {
    toDelete = {GerberAttribute::unset(QString())};
  }

  // Update dictionary, but don't add file-scoped attributes since they are
  // handled quite special.
  mDictionary.clear();
  foreach (const GerberAttribute& a, attributes) {
    if ((a.getType() == GerberAttribute::Type::Aperture) ||
        (a.getType() == GerberAttribute::Type::Object)) {
      mDictionary.append(a);
    }
  }

  // Build string.
  QString s;
  foreach (const GerberAttribute& a, toDelete) { s += a.toGerberString(); }
  foreach (const GerberAttribute& a, toSet) { s += a.toGerberString(); }
  return s;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
