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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDDRAGSELECTEDFOOTPRINTITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDDRAGSELECTEDFOOTPRINTITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../packageeditorstate.h"

#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CmdCircleEdit;
class CmdStrokeTextEdit;
class CmdPolygonEdit;
class CmdHoleEdit;

namespace library {

class CmdFootprintPadEdit;

namespace editor {

/*******************************************************************************
 *  Class CmdDragSelectedFootprintItems
 ******************************************************************************/

/**
 * @brief The CmdDragSelectedFootprintItems class
 */
class CmdDragSelectedFootprintItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdDragSelectedFootprintItems() = delete;
  CmdDragSelectedFootprintItems(const CmdDragSelectedFootprintItems& other) =
      delete;
  explicit CmdDragSelectedFootprintItems(
      const PackageEditorState::Context& context) noexcept;
  ~CmdDragSelectedFootprintItems() noexcept;

  // General Methods
  void setDeltaToStartPos(const Point& delta) noexcept;
  void translate(const Point& deltaPos) noexcept;
  void rotate(const Angle& angle) noexcept;
  void mirrorGeometry(Qt::Orientation orientation) noexcept;
  void mirrorLayer() noexcept;

  // Operator Overloadings
  CmdDragSelectedFootprintItems& operator       =(
      const CmdDragSelectedFootprintItems& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  void deleteAllCommands() noexcept;

  // Private Member Variables
  const PackageEditorState::Context& mContext;
  Point                              mCenterPos;
  Point                              mDeltaPos;
  Angle                              mDeltaRot;
  bool                               mMirroredGeometry;
  bool                               mMirroredLayer;

  // Move commands
  QList<CmdFootprintPadEdit*> mPadEditCmds;
  QList<CmdCircleEdit*>       mCircleEditCmds;
  QList<CmdPolygonEdit*>      mPolygonEditCmds;
  QList<CmdStrokeTextEdit*>   mTextEditCmds;
  QList<CmdHoleEdit*>         mHoleEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDDRAGSELECTEDFOOTPRINTITEMS_H
