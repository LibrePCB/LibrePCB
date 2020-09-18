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

#ifndef LIBREPCB_PROJECT_CMDBOARDNETSEGMENTADDELEMENTS_H
#define LIBREPCB_PROJECT_CMDBOARDNETSEGMENTADDELEMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../items/bi_via.h"

#include <librepcb/common/undocommand.h>
#include <librepcb/common/units/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class BI_NetSegment;
class BI_NetPoint;
class BI_NetLine;
class BI_NetLineAnchor;

/*******************************************************************************
 *  Class CmdBoardNetSegmentAddElements
 ******************************************************************************/

/**
 * @brief The CmdBoardNetSegmentAddElements class
 */
class CmdBoardNetSegmentAddElements final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdBoardNetSegmentAddElements(BI_NetSegment& segment) noexcept;
  ~CmdBoardNetSegmentAddElements() noexcept;

  // General Methods
  BI_Via* addVia(BI_Via& via);
  BI_Via* addVia(const Via& via);
  BI_NetPoint* addNetPoint(BI_NetPoint& netpoint);
  BI_NetPoint* addNetPoint(const Point& position);
  BI_NetLine* addNetLine(BI_NetLine& netline);
  BI_NetLine* addNetLine(BI_NetLineAnchor& startPoint,
                         BI_NetLineAnchor& endPoint, GraphicsLayer& layer,
                         const PositiveLength& width);

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables
  BI_NetSegment& mNetSegment;
  QList<BI_Via*> mVias;
  QList<BI_NetPoint*> mNetPoints;
  QList<BI_NetLine*> mNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDBOARDNETSEGMENTADDELEMENTS_H
