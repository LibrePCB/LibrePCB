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

#ifndef LIBREPCB_EDITOR_CMDCIRCLEEDIT_H
#define LIBREPCB_EDITOR_CMDCIRCLEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"
#include "cmdlistelementinsert.h"
#include "cmdlistelementremove.h"
#include "cmdlistelementsswap.h"

#include <librepcb/core/geometry/circle.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

namespace editor {

/*******************************************************************************
 *  Class CmdCircleEdit
 ******************************************************************************/

/**
 * @brief The CmdCircleEdit class
 */
class CmdCircleEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdCircleEdit() = delete;
  CmdCircleEdit(const CmdCircleEdit& other) = delete;
  explicit CmdCircleEdit(Circle& circle) noexcept;
  ~CmdCircleEdit() noexcept;

  // Setters
  void setLayer(const Layer& layer, bool immediate) noexcept;
  void setLineWidth(const UnsignedLength& width, bool immediate) noexcept;
  void setIsFilled(bool filled, bool immediate) noexcept;
  void setIsGrabArea(bool grabArea, bool immediate) noexcept;
  void setDiameter(const PositiveLength& dia, bool immediate) noexcept;
  void setCenter(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirrorGeometry(Qt::Orientation orientation, const Point& center,
                      bool immediate) noexcept;
  void mirrorLayer(bool immediate) noexcept;

  // Operator Overloadings
  CmdCircleEdit& operator=(const CmdCircleEdit& rhs) = delete;

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
  Circle& mCircle;

  // General Attributes
  const Layer* mOldLayer;
  const Layer* mNewLayer;
  UnsignedLength mOldLineWidth;
  UnsignedLength mNewLineWidth;
  bool mOldIsFilled;
  bool mNewIsFilled;
  bool mOldIsGrabArea;
  bool mNewIsGrabArea;
  PositiveLength mOldDiameter;
  PositiveLength mNewDiameter;
  Point mOldCenter;
  Point mNewCenter;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdCircleInsert =
    CmdListElementInsert<Circle, CircleListNameProvider, Circle::Event>;
using CmdCircleRemove =
    CmdListElementRemove<Circle, CircleListNameProvider, Circle::Event>;
using CmdCirclesSwap =
    CmdListElementsSwap<Circle, CircleListNameProvider, Circle::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
