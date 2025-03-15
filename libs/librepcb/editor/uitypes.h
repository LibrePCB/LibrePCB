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

#ifndef LIBREPCB_EDITOR_UITYPES_H
#define LIBREPCB_EDITOR_UITYPES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"
#include "editorcommand.h"
#include "utils/editortoolbox.h"

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline ui::EditorCommand l2s(const EditorCommand& cmd,
                             ui::EditorCommand in) noexcept {
  QString text = cmd.getDisplayText();
  if (cmd.getFlags().testFlag(EditorCommand::Flag::OpensPopup)) {
    text += "...";
  }
  in.text = q2s(text);
  in.status_tip = q2s(cmd.getDescription());
  in.shortcut = q2s(cmd.getKeySequences().value(0).toString());
  return in;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
