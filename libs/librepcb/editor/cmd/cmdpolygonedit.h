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

#ifndef LIBREPCB_EDITOR_CMDPOLYGONEDIT_H
#define LIBREPCB_EDITOR_CMDPOLYGONEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"
#include "cmdlistelementinsert.h"
#include "cmdlistelementremove.h"
#include "cmdlistelementsswap.h"

#include <librepcb/core/geometry/polygon.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdPolygonEdit
 ******************************************************************************/

/**
 * @brief The CmdPolygonEdit class
 */
class CmdPolygonEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdPolygonEdit() = delete;
  CmdPolygonEdit(const CmdPolygonEdit& other) = delete;
  explicit CmdPolygonEdit(Polygon& polygon) noexcept;
  ~CmdPolygonEdit() noexcept;

  // Setters
  void setLayerName(const GraphicsLayerName& name, bool immediate) noexcept;
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

  // Operator Overloadings
  CmdPolygonEdit& operator=(const CmdPolygonEdit& rhs) = delete;

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
  Polygon& mPolygon;

  // General Attributes
  GraphicsLayerName mOldLayerName;
  GraphicsLayerName mNewLayerName;
  UnsignedLength mOldLineWidth;
  UnsignedLength mNewLineWidth;
  bool mOldIsFilled;
  bool mNewIsFilled;
  bool mOldIsGrabArea;
  bool mNewIsGrabArea;
  Path mOldPath;
  Path mNewPath;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdPolygonInsert =
    CmdListElementInsert<Polygon, PolygonListNameProvider, Polygon::Event>;
using CmdPolygonRemove =
    CmdListElementRemove<Polygon, PolygonListNameProvider, Polygon::Event>;
using CmdPolygonsSwap =
    CmdListElementsSwap<Polygon, PolygonListNameProvider, Polygon::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
