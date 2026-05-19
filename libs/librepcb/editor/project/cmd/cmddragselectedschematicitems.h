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

#ifndef LIBREPCB_EDITOR_CMDDRAGSELECTEDSCHEMATICITEMS_H
#define LIBREPCB_EDITOR_CMDDRAGSELECTEDSCHEMATICITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SI_NetLine;
class SI_NetLineAnchor;
class Schematic;

namespace editor {

class CmdImageEdit;
class CmdPolygonEdit;
class CmdSchematicBusJunctionEdit;
class CmdSchematicBusLabelEdit;
class CmdSchematicNetLabelEdit;
class CmdSchematicNetPointEdit;
class CmdSymbolInstanceEdit;
class CmdSymbolInstanceTextsReset;
class CmdTextEdit;
class SchematicGraphicsScene;

/*******************************************************************************
 *  Class CmdDragSelectedSchematicItems
 ******************************************************************************/

/**
 * @brief The CmdDragSelectedSchematicItems class
 */
class CmdDragSelectedSchematicItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdDragSelectedSchematicItems(SchematicGraphicsScene& scene,
                                const Point& startPos = Point()) noexcept;
  ~CmdDragSelectedSchematicItems() noexcept override;

  // General Methods
  void snapToGrid() noexcept;
  void resetAllTexts() noexcept;
  void setCurrentPosition(const Point& pos) noexcept;
  void rotate(const Angle& angle, bool aroundCurrentPosition) noexcept;
  void mirror(Qt::Orientation orientation, bool aroundCurrentPosition) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommandGroup::performPostExecution()
  void performPostExecution() noexcept override;

  // Private Member Variables
  Schematic& mSchematic;
  int mItemCount;
  Point mStartPos;
  Point mDeltaPos;
  Point mCenterPos;
  Angle mDeltaAngle;
  bool mSnappedToGrid;
  bool mMirrored;
  bool mTextsReset;

  // Move commands
  QList<CmdSymbolInstanceEdit*> mSymbolEditCmds;
  QList<CmdSymbolInstanceTextsReset*> mSymbolTextsResetCmds;
  QList<CmdSchematicBusJunctionEdit*> mBusJunctionEditCmds;
  QList<CmdSchematicBusLabelEdit*> mBusLabelEditCmds;
  QList<CmdSchematicNetPointEdit*> mNetPointEditCmds;
  QList<CmdSchematicNetLabelEdit*> mNetLabelEditCmds;
  QList<CmdPolygonEdit*> mPolygonEditCmds;
  QList<CmdTextEdit*> mTextEditCmds;
  QList<CmdImageEdit*> mImageEditCmds;

  struct StretchAxisMask {
    bool stretchX = false;
    bool stretchY = false;
  };

  /// Netpoints that should follow a moving pin along a single axis so the
  /// connecting wire stretches orthogonally during preview. stretchX/stretchY
  /// indicate which components of the drag delta apply to this netpoint.
  struct StretchNetPointEdit {
    CmdSchematicNetPointEdit* cmd;
    StretchAxisMask mask;
  };
  QList<StretchNetPointEdit> mStretchEdits;

  /// Fixed-anchor netlines which need a temporary bend point once the drag
  /// moves perpendicular to their original orientation.
  struct SplitNetLineEdit {
    SI_NetLine* line;
    SI_NetLineAnchor* movingAnchor;
    SI_NetLineAnchor* fixedAnchor;
    /// Axes of the drag which make this split necessary.
    StretchAxisMask triggerMask;
    /// Axes to apply to the inserted bend point. This follows the moving anchor
    /// only along the original netline axis, leaving the far side in place.
    StretchAxisMask bendMask;
    UndoCommand* splitCmd = nullptr;
    CmdSchematicNetPointEdit* stretchCmd = nullptr;
  };
  QList<SplitNetLineEdit> mSplitNetLineEdits;

  void activateNeededSplitNetLineEdits(const Point& delta) noexcept;
  void deactivateUnneededSplitNetLineEdits(const Point& delta) noexcept;
  void discardSplitNetLineEdits() noexcept;

  /// Drop the live stretch cmds and roll any preview shifts back to the
  /// pre-drag netpoint positions. Used both for the no-op-drag discard path
  /// and to invalidate the stretch heuristic when rotate/mirror happens
  /// mid-drag.
  void discardStretchEdit(CmdSchematicNetPointEdit* cmd) noexcept;
  void discardStretchEdits() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
