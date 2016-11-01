/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "firstrunwizard.h"
#include "ui_firstrunwizard.h"
#include "firstrunwizardpage_welcome.h"
#include "firstrunwizardpage_workspacepath.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace application {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FirstRunWizard::FirstRunWizard(QWidget *parent) noexcept :
    QWizard(parent), mUi(new Ui::FirstRunWizard)
{
    mUi->setupUi(this);

    // add pages
    addPage(new FirstRunWizardPage_Welcome());
    addPage(new FirstRunWizardPage_WorkspacePath());

    // set header logo
    setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/img/logo/48x48.png"));
}

FirstRunWizard::~FirstRunWizard() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool FirstRunWizard::getCreateNewWorkspace() const noexcept
{
    return field("CreateWorkspace").toBool();
}

FilePath FirstRunWizard::getWorkspaceFilePath() const noexcept
{
    if (getCreateNewWorkspace())
        return FilePath(field("CreateWorkspacePath").toString());
    else
        return FilePath(field("OpenWorkspacePath").toString());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace application
} // namespace librepcb
