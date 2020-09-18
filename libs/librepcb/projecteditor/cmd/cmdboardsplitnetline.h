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

#ifndef LIBREPCB_PROJECT_CMDSPLITNETLINE_H
#define LIBREPCB_PROJECT_CMDSPLITNETLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;

namespace project {

class BI_NetLine;
class BI_NetPoint;
class CmdBoardNetSegmentAddElements;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardSplitNetLine
 ******************************************************************************/

/**
 * @brief Undo command to split a ::librepcb::project::BI_NetLine
 *
 * Splits the BI_NetLine at the given position and creates a new
 * ::librepcb::project::BI_NetPoint. It is not checked, whether the split
 * position lies on the BI_NetLine or not.
 */
class CmdBoardSplitNetLine : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdBoardSplitNetLine(BI_NetLine& netline, const Point& pos) noexcept;
  ~CmdBoardSplitNetLine() noexcept;

  BI_NetPoint* getSplitPoint() noexcept { return mSplitPoint; };

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  BI_NetLine& mOldNetLine;  ///< The BI_NetLine to be split
  BI_NetPoint* mSplitPoint;  ///< The new BI_NetPoint at the split position
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDSPLITNETLINE_H
