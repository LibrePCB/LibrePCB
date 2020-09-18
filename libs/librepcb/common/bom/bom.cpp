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
#include "bom.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Bom::Bom(const QStringList& columns) noexcept : mColumns(columns), mItems() {
}

Bom::~Bom() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Bom::addItem(const QString& designator,
                  const QStringList& attributes) noexcept {
  Q_ASSERT(attributes.count() == mColumns.count());

  bool itemAdded = false;
  for (BomItem& item : mItems) {
    if (item.getAttributes() == attributes) {
      item.addDesignator(designator);
      itemAdded = true;
    }
  }

  if (!itemAdded) {
    mItems.append(BomItem(designator, attributes));
  }

  // Sort items by designator to improve readability of the BOM
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(mItems.begin(), mItems.end(),
            [&collator](const BomItem& lhs, const BomItem& rhs) {
              return collator(lhs.getDesignators().first(),
                              rhs.getDesignators().first());
            });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
