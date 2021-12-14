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

#include <librepcb/common/application.h>
#include <librepcb/common/fileio/asynccopyoperation.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizardContext::InitializeWorkspaceWizardContext(
    const FilePath& ws, QObject* parent) noexcept
  : QObject(parent), mWorkspacePath(ws), mVersionToImport() {
}

InitializeWorkspaceWizardContext::~InitializeWorkspaceWizardContext() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

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
  workspace::Workspace ws(mWorkspacePath);  // can throw
  ws.getSettings().applicationLocale.set(mAppLocale);
  ws.getSettings().defaultLengthUnit.set(mLengthUnit);
  ws.getSettings().libraryNormOrder.set(mLibraryNormOrder);
  ws.getSettings().userName.set(mUserName);
  ws.getSettings().saveToFile();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
