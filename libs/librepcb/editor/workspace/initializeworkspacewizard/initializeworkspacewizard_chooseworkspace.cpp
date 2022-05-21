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
#include "initializeworkspacewizard_chooseworkspace.h"

#include "../../dialogs/filedialog.h"
#include "ui_initializeworkspacewizard_chooseworkspace.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizard_ChooseWorkspace::
    InitializeWorkspaceWizard_ChooseWorkspace(
        InitializeWorkspaceWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::InitializeWorkspaceWizard_ChooseWorkspace) {
  mUi->setupUi(this);
  mUi->btnBrowse->setFixedSize(mUi->edtPath->height(), mUi->edtPath->height());
  connect(mUi->btnBrowse, &QPushButton::clicked, this, [this]() {
    QString filepath = FileDialog::getExistingDirectory(
        this, tr("Select Workspace Directory"));
    if (!filepath.isEmpty()) {
      mUi->edtPath->setText(filepath);
    }
  });
  connect(mUi->edtPath, &QLineEdit::textChanged, this,
          &InitializeWorkspaceWizard_ChooseWorkspace::completeChanged);
  connect(&mContext, &InitializeWorkspaceWizardContext::workspacePathChanged,
          this, [this]() {
            setFinalPage(true);
            setFinalPage(false);
          });

  // By default, the suggested workspace path is a subdirectory within the
  // user's home folder. However, depending on the deployment method, the
  // home folder might be some kind of sandboxed and/or even deleted when
  // uninstalling LibrePCB (e.g. like Snap packages), which would be a horrible
  // location to store the workspace. In these cases a more reasonable
  // (persistent) path can be specified by an environment variable.
  FilePath defaultWsPath(qgetenv("LIBREPCB_DEFAULT_WORKSPACE_PATH"));
  if (!defaultWsPath.isValid()) {
    defaultWsPath = FilePath(QDir::homePath()).getPathTo("LibrePCB-Workspace");
  }
  if (!mContext.getWorkspacePath().isValid()) {
    mUi->edtPath->setText(defaultWsPath.toNative());
  }
}

InitializeWorkspaceWizard_ChooseWorkspace::
    ~InitializeWorkspaceWizard_ChooseWorkspace() noexcept {
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

bool InitializeWorkspaceWizard_ChooseWorkspace::isComplete() const noexcept {
  FilePath path(mUi->edtPath->text());
  mContext.setWorkspacePath(path);

  bool complete = false;
  QString message;
  if (!path.isValid()) {
    message = tr("Please select a directory.");
  } else if (Workspace::isValidWorkspacePath(path)) {
    message = tr("Directory contains a valid workspace.");
    mContext.setCreateWorkspace(false);
    complete = true;
  } else if (((!path.isExistingDir()) && (!path.isExistingFile())) ||
             path.isEmptyDir()) {
    message = tr("New, empty workspace will be created.");
    mContext.setCreateWorkspace(true);
    complete = true;
  } else {
    message = tr("Directory is not empty!");
  }

  if (complete) {
    mUi->lblStatus->setText("<font color=\"green\">✔ " % message % "</font>");
  } else {
    mUi->lblStatus->setText("<font color=\"red\">✖ " % message % "</font>");
  }

  return complete;
}

int InitializeWorkspaceWizard_ChooseWorkspace::nextId() const noexcept {
  if (mContext.getFileFormatVersions().contains(qApp->getFileFormatVersion())) {
    return InitializeWorkspaceWizardContext::ID_None;
  } else if (mContext.getFileFormatVersions().count() > 0) {
    return InitializeWorkspaceWizardContext::ID_ChooseImportVersion;
  } else {
    return InitializeWorkspaceWizardContext::ID_ChooseSettings;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
