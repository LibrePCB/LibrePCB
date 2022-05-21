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
#include "initializeworkspacewizard.h"

#include "initializeworkspacewizard_chooseimportversion.h"
#include "initializeworkspacewizard_choosesettings.h"
#include "initializeworkspacewizard_chooseworkspace.h"
#include "initializeworkspacewizard_finalizeimport.h"
#include "initializeworkspacewizard_welcome.h"
#include "ui_initializeworkspacewizard.h"

#include <librepcb/core/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizard::InitializeWorkspaceWizard(
    const FilePath& workspacePath, bool skipWelcomePage,
    QWidget* parent) noexcept
  : QWizard(parent),
    mContext(workspacePath),
    mUi(new Ui::InitializeWorkspaceWizard) {
  mUi->setupUi(this);
  setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/img/logo/48x48.png"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));

  // Add pages.
  setPage(InitializeWorkspaceWizardContext::ID_Welcome,
          new InitializeWorkspaceWizard_Welcome(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_ChooseWorkspace,
          new InitializeWorkspaceWizard_ChooseWorkspace(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_ChooseImportVersion,
          new InitializeWorkspaceWizard_ChooseImportVersion(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_FinalizeImport,
          new InitializeWorkspaceWizard_FinalizeImport(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_ChooseSettings,
          new InitializeWorkspaceWizard_ChooseSettings(mContext));

  // Determine start page.
  if ((!workspacePath.isValid()) && (!skipWelcomePage)) {
    setStartId(InitializeWorkspaceWizardContext::ID_Welcome);
  } else if (!workspacePath.isValid()) {
    setStartId(InitializeWorkspaceWizardContext::ID_ChooseWorkspace);
  } else if (mContext.getFileFormatVersions().count() > 0) {
    setStartId(InitializeWorkspaceWizardContext::ID_ChooseImportVersion);
  } else {
    setStartId(InitializeWorkspaceWizardContext::ID_ChooseSettings);
  }
}

InitializeWorkspaceWizard::~InitializeWorkspaceWizard() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
