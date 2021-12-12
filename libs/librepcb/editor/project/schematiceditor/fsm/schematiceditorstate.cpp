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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../schematiceditor.h"

#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematiclayerprovider.h>
#include <librepcb/core/types/gridproperties.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditorState::SchematicEditorState(const Context& context,
                                           QObject* parent) noexcept
  : QObject(parent), mContext(context) {
}

SchematicEditorState::~SchematicEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

Schematic* SchematicEditorState::getActiveSchematic() noexcept {
  return mContext.editor.getActiveSchematic();
}

PositiveLength SchematicEditorState::getGridInterval() const noexcept {
  return mContext.editorGraphicsView.getGridProperties().getInterval();
}

const LengthUnit& SchematicEditorState::getDefaultLengthUnit() const noexcept {
  return mContext.workspace.getSettings().defaultLengthUnit.get();
}

QList<GraphicsLayer*> SchematicEditorState::getAllowedGeometryLayers() const
    noexcept {
  return mContext.project.getLayers().getLayers({
      GraphicsLayer::sSymbolOutlines,
      // GraphicsLayer::sSymbolHiddenGrabAreas, -> makes no sense in schematics
      GraphicsLayer::sSymbolNames,
      GraphicsLayer::sSymbolValues,
      GraphicsLayer::sSchematicSheetFrames,
      GraphicsLayer::sSchematicDocumentation,
      GraphicsLayer::sSchematicComments,
      GraphicsLayer::sSchematicGuide,
  });
}

bool SchematicEditorState::execCmd(UndoCommand* cmd) {
  return mContext.undoStack.execCmd(cmd);
}

QWidget* SchematicEditorState::parentWidget() noexcept {
  return &mContext.editor;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
