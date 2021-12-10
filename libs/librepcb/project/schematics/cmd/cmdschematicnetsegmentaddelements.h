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

#ifndef LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTADDELEMENTS_H
#define LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTADDELEMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommand.h>
#include <librepcb/common/units/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class SI_NetLine;
class SI_NetLineAnchor;
class SI_NetPoint;
class SI_NetSegment;

/*******************************************************************************
 *  Class CmdSchematicNetSegmentAddElements
 ******************************************************************************/

/**
 * @brief The CmdSchematicNetSegmentAddElements class
 */
class CmdSchematicNetSegmentAddElements final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdSchematicNetSegmentAddElements(SI_NetSegment& segment) noexcept;
  ~CmdSchematicNetSegmentAddElements() noexcept;

  // General Methods
  SI_NetPoint* addNetPoint(SI_NetPoint& netpoint);
  SI_NetPoint* addNetPoint(const Point& position);
  SI_NetLine* addNetLine(SI_NetLine& netline);
  SI_NetLine* addNetLine(SI_NetLineAnchor& startPoint,
                         SI_NetLineAnchor& endPoint);

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  SI_NetSegment& mNetSegment;
  QList<SI_NetPoint*> mNetPoints;
  QList<SI_NetLine*> mNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
