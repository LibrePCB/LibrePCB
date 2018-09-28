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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "firstrunwizardpage_workspacesettings.h"

#include "ui_firstrunwizardpage_workspacesettings.h"

#include <librepcb/common/systeminfo.h>
#include <librepcb/workspace/workspace.h>

#include <QFileDialog>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FirstRunWizardPage_WorkspaceSettings::FirstRunWizardPage_WorkspaceSettings(
    QWidget *parent) noexcept
  : QWizardPage(parent), mUi(new Ui::FirstRunWizardPage_WorkspaceSettings) {
  mUi->setupUi(this);
  registerField("NewWorkspaceUserName", mUi->edtUserName);

  // Initialize user name with the system's username.
  mUi->edtUserName->setText(SystemInfo::getFullUsername());
}

FirstRunWizardPage_WorkspaceSettings::
    ~FirstRunWizardPage_WorkspaceSettings() noexcept {
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

bool FirstRunWizardPage_WorkspaceSettings::validatePage() noexcept {
  return true;  // Any user name is valid
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
