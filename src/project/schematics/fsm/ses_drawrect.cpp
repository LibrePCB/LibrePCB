/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "ses_drawrect.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_DrawRect::SES_DrawRect(SchematicEditor& editor, Ui::SchematicEditor& editorUi) :
    SES_Base(editor, editorUi)
{
}

SES_DrawRect::~SES_DrawRect()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_DrawRect::process(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    return PassToParentState;
}

bool SES_DrawRect::entry(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolDrawRectangle->setCheckable(true);
    mEditorUi.actionToolDrawRectangle->setChecked(true);
    return true;
}

bool SES_DrawRect::exit(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolDrawRectangle->setCheckable(false);
    mEditorUi.actionToolDrawRectangle->setChecked(false);
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
