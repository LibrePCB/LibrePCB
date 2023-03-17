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

#ifndef LIBREPCB_EDITOR_CMDPASTEBOARDITEMS_H
#define LIBREPCB_EDITOR_CMDPASTEBOARDITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/types/point.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class NetSignal;
class Project;

namespace editor {

class BoardClipboardData;
class BoardGraphicsScene;

/*******************************************************************************
 *  Class CmdPasteBoardItems
 ******************************************************************************/

/**
 * @brief The CmdPasteBoardItems class
 */
class CmdPasteBoardItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdPasteBoardItems() = delete;
  CmdPasteBoardItems(const CmdPasteBoardItems& other) = delete;
  CmdPasteBoardItems(BoardGraphicsScene& scene,
                     std::unique_ptr<BoardClipboardData> data,
                     const Point& posOffset) noexcept;
  ~CmdPasteBoardItems() noexcept;

  // Operator Overloadings
  CmdPasteBoardItems& operator=(const CmdPasteBoardItems& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  NetSignal* getOrCreateNetSignal(const QString& name);

private:  // Data
  BoardGraphicsScene& mScene;
  Board& mBoard;
  Project& mProject;
  std::unique_ptr<BoardClipboardData> mData;
  Point mPosOffset;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
