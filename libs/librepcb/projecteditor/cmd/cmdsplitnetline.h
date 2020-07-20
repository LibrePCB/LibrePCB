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
 *  Class CmdBoardNetLineSplit
 ******************************************************************************/

class CmdSplitNetLine : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdSplitNetLine(BI_NetLine& netline, Point& pos) noexcept;
  ~CmdSplitNetLine() noexcept;

  BI_NetPoint* getSplitPoint() noexcept { return mSplitPoint; };

private: // Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  CmdBoardNetSegmentAddElements* mCmdAdd;
  BI_NetLine& mOldNetLine;
  Point& mSplitPosition;
  BI_NetPoint* mSplitPoint;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSPLITNETLINE_H
