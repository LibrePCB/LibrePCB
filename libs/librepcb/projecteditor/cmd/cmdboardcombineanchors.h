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

#ifndef LIBREPCB_PROJECT_EDITOR_CMDBOARDCOMBINEANCHORS_H
#define LIBREPCB_PROJECT_EDITOR_CMDBOARDCOMBINEANCHORS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class BI_NetLineAnchor;
class BI_NetPoint;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardSplitNetLine
 ******************************************************************************/

/**
 * @brief Undo command to combine two ::librepcb::project::BI_NetLineAnchor
 */
class CmdBoardCombineAnchors : public UndoCommandGroup {
public:
  // Constructors / Destructor
  /**
   * @brief CmdBoardCombineAnchors
   *
   * At least one of the anchors _must_ be a ::librepcb::project::BI_NetPoint
   * since only these can be gracefully removed.
   *
   * @param removeAnchor The anchor to be removed. If this is not a
   * ::librepcb::project::BI_NetPoint, keepAnchor will be chosen for removal.
   * @param keepAnchor The anchor to be kept. May be chosen for removal if
   * removeAnchor is not a ::librepcb::project::BI_NetPoint.
   *
   * @throws LogicError Error when no anchor is a
   * ::librepcb::project::BI_NetPoint
   */
  explicit CmdBoardCombineAnchors(BI_NetLineAnchor& removeAnchor,
                                  BI_NetLineAnchor& keepAnchor);
  ~CmdBoardCombineAnchors() noexcept;

  /**
   * @brief Get the anchor that will be kept after removal.
   * @return The anchor that will be kept.
   */
  BI_NetLineAnchor* getKeepAnchor() { return mKeepAnchor; }

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  BI_NetPoint* mRemovePoint;  ///< The point that will be removed. Is nullptr if
                              ///< it would be identical to mKeepAnchor.
  BI_NetLineAnchor* mKeepAnchor;  ///< The anchor that will be kept.
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_EDITOR_CMDBOARDCOMBINEANCHORS_H
