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

#ifndef LIBREPCB_PROJECT_CMDBOARDVIAEDIT_H
#define LIBREPCB_PROJECT_CMDBOARDVIAEDIT_H

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

class NetSignal;

/*******************************************************************************
 *  Class CmdBoardViaEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardViaEdit class
 */
class CmdBoardViaEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdBoardViaEdit(BI_Via& via) noexcept;
  ~CmdBoardViaEdit() noexcept;

  // Setters
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void setShape(BI_Via::Shape shape, bool immediate) noexcept;
  void setSize(const PositiveLength& size, bool immediate) noexcept;
  void setDrillDiameter(const PositiveLength& diameter,
                        bool                  immediate) noexcept;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  BI_Via& mVia;

  // General Attributes
  Point          mOldPos;
  Point          mNewPos;
  BI_Via::Shape  mOldShape;
  BI_Via::Shape  mNewShape;
  PositiveLength mOldSize;
  PositiveLength mNewSize;
  PositiveLength mOldDrillDiameter;
  PositiveLength mNewDrillDiameter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDBOARDVIAEDIT_H
