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
#include "pickplacedata.h"

#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PickPlaceData::PickPlaceData(const QString& projectName,
                             const QString& projectVersion,
                             const QString& boardName) noexcept
  : mProjectName(projectName),
    mProjectVersion(projectVersion),
    mBoardName(boardName),
    mItems() {
}

PickPlaceData::~PickPlaceData() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PickPlaceData::addItem(const PickPlaceDataItem& item) noexcept {
  mItems.append(item);

  // Sort items by designator to improve readability of the exported file.
  Toolbox::sortNumeric(
      mItems,
      [](const QCollator& cmp, const PickPlaceDataItem& lhs,
         const PickPlaceDataItem& rhs) {
        return cmp(lhs.getDesignator(), rhs.getDesignator());
      },
      Qt::CaseInsensitive, false);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
