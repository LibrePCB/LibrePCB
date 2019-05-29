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

#ifndef LIBREPCB_LIBRARY_CMDFOOTPRINTPADEDIT_H
#define LIBREPCB_LIBRARY_CMDFOOTPRINTPADEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../footprintpad.h"

#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class CmdFootprintPadEdit
 ******************************************************************************/

/**
 * @brief The CmdFootprintPadEdit class
 */
class CmdFootprintPadEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdFootprintPadEdit()                                 = delete;
  CmdFootprintPadEdit(const CmdFootprintPadEdit& other) = delete;
  explicit CmdFootprintPadEdit(FootprintPad& pad) noexcept;
  ~CmdFootprintPadEdit() noexcept;

  // Setters
  void setPackagePadUuid(const Uuid& pad, bool immediate) noexcept;
  void setBoardSide(FootprintPad::BoardSide side, bool immediate) noexcept;
  void setShape(FootprintPad::Shape shape, bool immediate) noexcept;
  void setWidth(const PositiveLength& width, bool immediate) noexcept;
  void setHeight(const PositiveLength& height, bool immediate) noexcept;
  void setDrillDiameter(const UnsignedLength& dia, bool immediate) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

  // Operator Overloadings
  CmdFootprintPadEdit& operator=(const CmdFootprintPadEdit& rhs) = delete;

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
  FootprintPad& mPad;

  // General Attributes
  Uuid                    mOldPackagePadUuid;
  Uuid                    mNewPackagePadUuid;
  FootprintPad::BoardSide mOldBoardSide;
  FootprintPad::BoardSide mNewBoardSide;
  FootprintPad::Shape     mOldShape;
  FootprintPad::Shape     mNewShape;
  PositiveLength          mOldWidth;
  PositiveLength          mNewWidth;
  PositiveLength          mOldHeight;
  PositiveLength          mNewHeight;
  Point                   mOldPos;
  Point                   mNewPos;
  Angle                   mOldRotation;
  Angle                   mNewRotation;
  UnsignedLength          mOldDrillDiameter;
  UnsignedLength          mNewDrillDiameter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_CMDFOOTPRINTPADEDIT_H
