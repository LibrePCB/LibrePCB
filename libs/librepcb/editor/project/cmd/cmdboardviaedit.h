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

#ifndef LIBREPCB_EDITOR_CMDBOARDVIAEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDVIAEDIT_H

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

class NetSignal;

namespace editor {

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
  void setLayers(const Layer& startLayer, const Layer& endLayer) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirrorLayers(int innerLayers) noexcept;
  void setDrillAndSize(const std::optional<PositiveLength>& drill,
                       const std::optional<PositiveLength>& size,
                       bool immediate);
  void setExposureConfig(const MaskConfig& config) noexcept;

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
  BI_Via& mVia;

  // General Attributes
  const Layer* mOldStartLayer;
  const Layer* mNewStartLayer;
  const Layer* mOldEndLayer;
  const Layer* mNewEndLayer;
  Point mOldPos;
  Point mNewPos;
  std::optional<PositiveLength> mOldDrillDiameter;
  std::optional<PositiveLength> mNewDrillDiameter;
  std::optional<PositiveLength> mOldSize;
  std::optional<PositiveLength> mNewSize;
  MaskConfig mOldExposureConfig;
  MaskConfig mNewExposureConfig;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
