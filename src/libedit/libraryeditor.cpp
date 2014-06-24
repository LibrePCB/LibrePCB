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
#include "libraryeditor.h"
#include "ui_libraryeditor.h"
#include "../workspace/workspace.h"

namespace libedit{

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryEditor::LibraryEditor(Workspace* workspace) :
    QMainWindow(0), ui(new Ui::LibraryEditor), mWorkspace(workspace)
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

} // namespace libedit
