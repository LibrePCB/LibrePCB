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

#ifndef LIBREPCB_EDITOR_CMDBOARDNETSEGMENTADDELEMENTS_H
#define LIBREPCB_EDITOR_CMDBOARDNETSEGMENTADDELEMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_NetLine;
class BI_NetLineAnchor;
class BI_NetPoint;
class BI_NetSegment;
class Layer;

namespace editor {

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
  BI_NetLine* addNetLine(BI_NetLineAnchor& a, BI_NetLineAnchor& b,
                         const Layer& layer, const PositiveLength& width);

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
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

}  // namespace editor
}  // namespace librepcb

#endif
