/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_CMDBOARDNETSEGMENTREMOVEELEMENTS_H
#define LIBREPCB_PROJECT_CMDBOARDNETSEGMENTREMOVEELEMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class BI_NetSegment;
class BI_Via;
class BI_NetPoint;
class BI_NetLine;

/*******************************************************************************
 *  Class CmdBoardNetSegmentRemoveElements
 ******************************************************************************/

/**
 * @brief The CmdBoardNetSegmentRemoveElements class
 */
class CmdBoardNetSegmentRemoveElements final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdBoardNetSegmentRemoveElements(BI_NetSegment& segment) noexcept;
  ~CmdBoardNetSegmentRemoveElements() noexcept;

  // General Methods
  void removeVia(BI_Via& via);
  void removeNetPoint(BI_NetPoint& netpoint);
  void removeNetLine(BI_NetLine& netline);

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables
  BI_NetSegment&      mNetSegment;
  QList<BI_Via*>      mVias;
  QList<BI_NetPoint*> mNetPoints;
  QList<BI_NetLine*>  mNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDBOARDNETSEGMENTREMOVEELEMENTS_H
