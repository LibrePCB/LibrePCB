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
#include "initializeworkspacewizardcontext.h"

#include <librepcb/core/application.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizardContext::InitializeWorkspaceWizardContext(
    QObject* parent) noexcept
  : QObject(parent),
    mWorkspacePath(),
    mWorkspacePathValid(false),
    mWorkspaceExists(false),
    mDataDirs(),
    mDataDir(),
    mUpgradeCopyDirs(),
    mAppLocale(),
    mLengthUnit(),
    mLibraryNormOrder(),
    mUserName() {
  mDataDir = Workspace::determineDataDirectory(
      mDataDirs, mUpgradeCopyDirs.first, mUpgradeCopyDirs.second);
}

InitializeWorkspaceWizardContext::~InitializeWorkspaceWizardContext() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool InitializeWorkspaceWizardContext::getWorkspaceContainsNewerFileFormats()
    const noexcept {
  return (!mDataDirs.isEmpty()) &&
      (qApp->getFileFormatVersion() <
       Toolbox::sorted(mDataDirs.values()).last());
}

void InitializeWorkspaceWizardContext::setWorkspacePath(const FilePath& fp) {
  if (!fp.isValid()) {
    mWorkspacePathValid = false;
    mWorkspaceExists = false;
    mDataDirs.clear();
  } else if (Workspace::checkCompatibility(fp)) {
    mDataDirs = Workspace::findDataDirectories(fp);  // can throw
    mWorkspacePathValid = true;
    mWorkspaceExists = true;
  } else if (((!fp.isExistingDir()) && (!fp.isExistingFile())) ||
             fp.isEmptyDir()) {
    mWorkspacePathValid = true;
    mWorkspaceExists = false;
    mDataDirs.clear();
  } else {
    mWorkspacePathValid = false;
    mWorkspaceExists = false;
    mDataDirs.clear();
  }
  mDataDir = Workspace::determineDataDirectory(
      mDataDirs, mUpgradeCopyDirs.first, mUpgradeCopyDirs.second);
  mWorkspacePath = fp;
}

void InitializeWorkspaceWizardContext::initializeEmptyWorkspace() const {
  if (!mWorkspaceExists) {
    Workspace::createNewWorkspace(mWorkspacePath);  // can throw
  }
  Workspace ws(mWorkspacePath, mDataDir);  // can throw
  ws.getSettings().applicationLocale.set(mAppLocale);
  ws.getSettings().defaultLengthUnit.set(mLengthUnit);
  ws.getSettings().libraryNormOrder.set(mLibraryNormOrder);
  ws.getSettings().userName.set(mUserName);
  ws.saveSettings();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
