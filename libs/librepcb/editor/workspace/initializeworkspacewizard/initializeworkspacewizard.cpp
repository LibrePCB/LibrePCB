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

#include "initializeworkspacewizard_choosesettings.h"
#include "initializeworkspacewizard_chooseworkspace.h"
#include "initializeworkspacewizard_upgrade.h"
#include "initializeworkspacewizard_welcome.h"
#include "ui_initializeworkspacewizard.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizard::InitializeWorkspaceWizard(bool forceChoosePath,
                                                     QWidget* parent) noexcept
  : QWizard(parent),
    mContext(),
    mUi(new Ui::InitializeWorkspaceWizard),
    mForceChoosePath(forceChoosePath),
    mNeedsToBeShown(true) {
  mUi->setupUi(this);
  setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/img/logo/48x48.png"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));

  // Add pages.
  setPage(InitializeWorkspaceWizardContext::ID_Welcome,
          new InitializeWorkspaceWizard_Welcome(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_ChooseWorkspace,
          new InitializeWorkspaceWizard_ChooseWorkspace(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_Upgrade,
          new InitializeWorkspaceWizard_Upgrade(mContext));
  setPage(InitializeWorkspaceWizardContext::ID_ChooseSettings,
          new InitializeWorkspaceWizard_ChooseSettings(mContext));
  updateStartPage();
}

InitializeWorkspaceWizard::~InitializeWorkspaceWizard() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void InitializeWorkspaceWizard::setWorkspacePath(const FilePath& fp) {
  mContext.setWorkspacePath(fp);  // can throw
  updateStartPage();

  if (mNeedsToBeShown && (!mForceChoosePath)) {
    switch (startId()) {
      case InitializeWorkspaceWizardContext::ID_Welcome:
        qInfo() << "No workspace selected, asking for path...";
        break;
      case InitializeWorkspaceWizardContext::ID_ChooseWorkspace:
        qInfo() << "Invalid workspace selected, asking for different path...";
        break;
      case InitializeWorkspaceWizardContext::ID_Upgrade:
        qInfo() << "Workspace data is outdated, asking for upgrade...";
        break;
      case InitializeWorkspaceWizardContext::ID_ChooseSettings:
        qInfo() << "Workspace data not initialized, asking for settings...";
        break;
      default:
        break;
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void InitializeWorkspaceWizard::updateStartPage() noexcept {
  mNeedsToBeShown = true;
  if ((!mContext.getWorkspacePath().isValid()) && (!mForceChoosePath)) {
    setStartId(InitializeWorkspaceWizardContext::ID_Welcome);
  } else if (mForceChoosePath || (!mContext.getWorkspaceExists())) {
    setStartId(InitializeWorkspaceWizardContext::ID_ChooseWorkspace);
  } else if (mContext.getNeedsUpgrade()) {
    setStartId(InitializeWorkspaceWizardContext::ID_Upgrade);
  } else if (mContext.getNeedsInitialization()) {
    setStartId(InitializeWorkspaceWizardContext::ID_ChooseSettings);
  } else {
    setStartId(InitializeWorkspaceWizardContext::ID_None);
    mNeedsToBeShown = false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
