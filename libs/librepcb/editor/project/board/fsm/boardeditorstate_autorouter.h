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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_AUTOROUTER_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_AUTOROUTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class BoardEditorState_Autorouter
 ******************************************************************************/

/**
 * @brief The "autorouter" state/tool of the board editor
 */
class BoardEditorState_Autorouter final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_Autorouter() = delete;
  BoardEditorState_Autorouter(const BoardEditorState_Autorouter& other) =
      delete;
  explicit BoardEditorState_Autorouter(const Context& context) noexcept;
  ~BoardEditorState_Autorouter() noexcept override;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processAbortCommand() noexcept override;

  // Connection to UI
  void start() noexcept;

  // Operator Overloadings
  BoardEditorState_Autorouter& operator=(
      const BoardEditorState_Autorouter& rhs) = delete;

private:  // Data
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
