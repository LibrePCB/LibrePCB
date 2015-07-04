/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "ses_base.h"
#include "../schematiceditor.h"
#include <librepcbproject/project.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Base::SES_Base(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                   GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    QObject(0), mProject(editor.getProject()), mCircuit(editor.getProject().getCircuit()),
    mEditor(editor), mEditorUi(editorUi), mEditorGraphicsView(editorGraphicsView),
    mUndoStack(undoStack)
{
}

SES_Base::~SES_Base()
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
