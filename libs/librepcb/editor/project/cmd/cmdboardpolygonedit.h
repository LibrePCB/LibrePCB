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

#ifndef LIBREPCB_EDITOR_CMDBOARDPOLYGONEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDPOLYGONEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/project/board/boardpolygondata.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class BI_Polygon;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardPolygonEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardPolygonEdit class
 */
class CmdBoardPolygonEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdBoardPolygonEdit() = delete;
  CmdBoardPolygonEdit(const CmdBoardPolygonEdit& other) = delete;
  explicit CmdBoardPolygonEdit(BI_Polygon& polygon) noexcept;
  ~CmdBoardPolygonEdit() noexcept;

  // Setters
  void setLayer(const Layer& layer, bool immediate) noexcept;
  void setLineWidth(const UnsignedLength& width, bool immediate) noexcept;
  void setIsFilled(bool filled, bool immediate) noexcept;
  void setIsGrabArea(bool grabArea, bool immediate) noexcept;
  void setPath(const Path& path, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirrorGeometry(Qt::Orientation orientation, const Point& center,
                      bool immediate) noexcept;
  void mirrorLayer(bool immediate) noexcept;
  void setLocked(bool locked) noexcept;

  // Operator Overloadings
  CmdBoardPolygonEdit& operator=(const CmdBoardPolygonEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  BI_Polygon& mPolygon;
  BoardPolygonData mOldData;
  BoardPolygonData mNewData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
