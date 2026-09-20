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

// Note: This file was modified with the assistance of Claude AI.
// Modifications were reviewed by Avetos Design on 2026-09-19.

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

class SI_BusSegment;
class SI_NetSegment;
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
  const QSet<SI_NetSegment*>& getModifiedNetSegments() const noexcept {
    return mModifiedNetSegments;
  }
  const QSet<SI_BusSegment*>& getModifiedBusSegments() const noexcept {
    return mModifiedBusSegments;
  }

private:
  // Private Methods

  /// Delete all unexecuted edit commands (reverting their changes)
  void deleteAllCommands() noexcept;

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

  // Segments whose geometry might have been modified by the drag.
  QSet<SI_NetSegment*> mModifiedNetSegments;
  QSet<SI_BusSegment*> mModifiedBusSegments;

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
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
