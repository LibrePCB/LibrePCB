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
#include "boardeditorstate.h"

#include "../boardeditor.h"

#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState::BoardEditorState(const Context& context,
                                   QObject* parent) noexcept
  : QObject(parent), mContext(context) {
}

BoardEditorState::~BoardEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

Board* BoardEditorState::getActiveBoard() noexcept {
  return mContext.editor.getActiveBoard();
}

PositiveLength BoardEditorState::getGridInterval() const noexcept {
  return mContext.editorGraphicsView.getGridProperties().getInterval();
}

const LengthUnit& BoardEditorState::getDefaultLengthUnit() const noexcept {
  return mContext.workspace.getSettings().defaultLengthUnit.get();
}

bool BoardEditorState::execCmd(UndoCommand* cmd) {
  return mContext.undoStack.execCmd(cmd);
}

QWidget* BoardEditorState::parentWidget() noexcept {
  return &mContext.editor;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
