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
#include <QtWidgets>
#include <QFileDialog>
#include "controlpanel.h"
#include "ui_controlpanel.h"

#include "../workspace.h"
#include "../../project/project.h"

using namespace project;


/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ControlPanel::ControlPanel(Workspace* workspace) :
    QMainWindow(0), ui(new Ui::ControlPanel), mWorkspace(workspace)
{
    ui->setupUi(this);

    setWindowTitle("EDA4U Control Panel - " + mWorkspace->getWorkspaceDir().absolutePath());
}

ControlPanel::~ControlPanel()
{
    delete ui;              ui = 0;
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
