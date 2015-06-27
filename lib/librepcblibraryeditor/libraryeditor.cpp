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
#include "libraryeditor.h"
#include "ui_libraryeditor.h"
#include "../workspace/workspace.h"

namespace library_editor{

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryEditor::LibraryEditor() :
    QMainWindow(0), ui(new Ui::LibraryEditor)
{
    ui->setupUi(this);
}

LibraryEditor::~LibraryEditor()
{
    delete ui;              ui = 0;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library_editor
