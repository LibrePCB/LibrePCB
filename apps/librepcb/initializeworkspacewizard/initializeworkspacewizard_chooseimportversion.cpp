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
#include "initializeworkspacewizard_chooseimportversion.h"

#include "librepcb/workspace/workspace.h"
#include "ui_initializeworkspacewizard_chooseimportversion.h"

#include <librepcb/common/application.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

using namespace workspace;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizard_ChooseImportVersion::
    InitializeWorkspaceWizard_ChooseImportVersion(
        InitializeWorkspaceWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::InitializeWorkspaceWizard_ChooseImportVersion) {
  mUi->setupUi(this);

  mUi->cbxVersions->addItem(tr("Do not import any data"));
  foreach (const Version& version,
           Workspace::getFileFormatVersionsOfWorkspace(
               mContext.getWorkspacePath())) {
    if (version < qApp->getFileFormatVersion()) {
      mUi->cbxVersions->addItem("LibrePCB " % version.toStr() % ".x",
                                version.toStr());
    }
  }
  mUi->cbxVersions->setCurrentIndex(mUi->cbxVersions->count() - 1);
}

InitializeWorkspaceWizard_ChooseImportVersion::
    ~InitializeWorkspaceWizard_ChooseImportVersion() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool InitializeWorkspaceWizard_ChooseImportVersion::validatePage() noexcept {
  mContext.setVersionToImport(
      Version::tryFromString(mUi->cbxVersions->currentData().toString()));
  return true;
}

int InitializeWorkspaceWizard_ChooseImportVersion::nextId() const noexcept {
  if (mContext.getVersionToImport()) {
    return InitializeWorkspaceWizardContext::ID_FinalizeImport;
  } else {
    return InitializeWorkspaceWizardContext::ID_ChooseSettings;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
