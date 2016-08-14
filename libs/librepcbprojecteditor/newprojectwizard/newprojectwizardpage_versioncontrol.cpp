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
#include <QtCore>
#include <QtWidgets>
#include "newprojectwizardpage_versioncontrol.h"
#include "ui_newprojectwizardpage_versioncontrol.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NewProjectWizardPage_VersionControl::NewProjectWizardPage_VersionControl(QWidget *parent) noexcept :
    QWizardPage(parent), mUi(new Ui::NewProjectWizardPage_VersionControl)
{
    mUi->setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/img/actions/plus_2.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));
}

NewProjectWizardPage_VersionControl::~NewProjectWizardPage_VersionControl() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool NewProjectWizardPage_VersionControl::getInitGitRepository() const noexcept
{
    return mUi->gbxInitGitRepo->isChecked();
}

/*****************************************************************************************
 *  GUI Action Handlers
 ****************************************************************************************/



/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/



/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
