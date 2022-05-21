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
#include <librepcb/core/fileio/asynccopyoperation.h>
#include <librepcb/core/fileio/fileutils.h>
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
    const FilePath& ws, QObject* parent) noexcept
  : QObject(parent),
    mWorkspacePath(),
    mCreateWorkspace(false),
    mVersionToImport() {
  setWorkspacePath(ws);
}

InitializeWorkspaceWizardContext::~InitializeWorkspaceWizardContext() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void InitializeWorkspaceWizardContext::setWorkspacePath(
    const FilePath& fp) noexcept {
  if (fp != mWorkspacePath) {
    mWorkspacePath = fp;
    mFileFormatVersions = Workspace::getFileFormatVersionsOfWorkspace(fp);
    emit workspacePathChanged();
  }
}

std::unique_ptr<AsyncCopyOperation>
    InitializeWorkspaceWizardContext::createImportCopyOperation() const
    noexcept {
  if (mVersionToImport) {
    FilePath src = mWorkspacePath.getPathTo("v" % mVersionToImport->toStr());
    FilePath dst =
        mWorkspacePath.getPathTo("v" % qApp->getFileFormatVersion().toStr());
    return std::unique_ptr<AsyncCopyOperation>(
        new AsyncCopyOperation(src, dst));
  } else {
    return std::unique_ptr<AsyncCopyOperation>(nullptr);
  }
}

void InitializeWorkspaceWizardContext::initializeEmptyWorkspace() const {
  if (mCreateWorkspace) {
    Workspace::createNewWorkspace(mWorkspacePath);  // can throw
  }
  Workspace ws(mWorkspacePath);  // can throw
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
