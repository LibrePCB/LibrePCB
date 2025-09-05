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

#ifndef LIBREPCB_EDITOR_CMDBOARDPADEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDPADEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardPadEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardPadEdit class
 */
class CmdBoardPadEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdBoardPadEdit(BI_Pad& pad) noexcept;
  ~CmdBoardPadEdit() noexcept;

  // Setters
  void setComponentSideAndHoles(Pad::ComponentSide side,
                                const PadHoleList& holes, bool immediate);
  void setFunction(Pad::Function function, bool immediate) noexcept;
  void setShape(Pad::Shape shape, bool immediate) noexcept;
  void setWidth(const PositiveLength& width, bool immediate) noexcept;
  void setHeight(const PositiveLength& height, bool immediate) noexcept;
  void setRadius(const UnsignedLimitedRatio& radius, bool immediate) noexcept;
  void setCustomShapeOutline(const Path& outline) noexcept;
  void setStopMaskConfig(const MaskConfig& config, bool immediate) noexcept;
  void setSolderPasteConfig(const MaskConfig& config) noexcept;
  void setCopperClearance(const UnsignedLength& clearance,
                          bool immediate) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirror(const Point& center, Qt::Orientation orientation, bool immediate);
  void setLocked(bool locked) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  BI_Pad& mPad;

  // General Attributes
  BoardPadData mOldProperties;
  BoardPadData mNewProperties;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
