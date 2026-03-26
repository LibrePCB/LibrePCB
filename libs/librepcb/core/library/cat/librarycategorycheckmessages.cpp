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
#include "librarycategorycheckmessages.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  MsgInvalidParent
 ******************************************************************************/

MsgInvalidParent::MsgInvalidParent() noexcept
  : RuleCheckMessage(Severity::Error, tr("Invalid parent category"),
                     tr("The category has assigned itself as its parent "
                        "category, which leads to an endless recursion and is "
                        "thus invalid. Assign a different parent category."),
                     "invalid_parent") {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
