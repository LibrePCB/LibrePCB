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

#ifndef LIBREPCB_UITYPES_H
#define LIBREPCB_UITYPES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/workspace/theme.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline Theme::GridStyle s2l(ui::GridStyle gs) noexcept {
  if (gs == ui::GridStyle::Lines) {
    return Theme::GridStyle::Lines;
  } else if (gs == ui::GridStyle::Dots) {
    return Theme::GridStyle::Dots;
  } else {
    return Theme::GridStyle::None;
  }
}

inline ui::GridStyle l2s(Theme::GridStyle gs) noexcept {
  if (gs == Theme::GridStyle::Lines) {
    return ui::GridStyle::Lines;
  } else if (gs == Theme::GridStyle::Dots) {
    return ui::GridStyle::Dots;
  } else {
    return ui::GridStyle::None;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
