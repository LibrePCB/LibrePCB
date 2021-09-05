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
#include "firstrunwizardpage_workspacepath.h"

#include "ui_firstrunwizardpage_workspacepath.h"

#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FirstRunWizardPage_WorkspacePath::FirstRunWizardPage_WorkspacePath(
    QWidget* parent) noexcept
  : QWizardPage(parent), mUi(new Ui::FirstRunWizardPage_WorkspacePath) {
  mUi->setupUi(this);
  registerField("CreateWorkspace", mUi->rbtnCreateWs);
  registerField("CreateWorkspacePath", mUi->edtCreateWsPath);
  registerField("OpenWorkspace", mUi->rbtnOpenWs);
  registerField("OpenWorkspacePath", mUi->edtOpenWsPath);

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
  mUi->edtCreateWsPath->setText(defaultWsPath.toNative());
  mUi->edtOpenWsPath->setText(defaultWsPath.toNative());
  if (workspace::Workspace::isValidWorkspacePath(defaultWsPath))
    mUi->rbtnOpenWs->setChecked(true);
}

FirstRunWizardPage_WorkspacePath::~FirstRunWizardPage_WorkspacePath() noexcept {
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

bool FirstRunWizardPage_WorkspacePath::validatePage() noexcept {
  if (field("CreateWorkspace").toBool()) {
    FilePath path(field("CreateWorkspacePath").toString());
    if ((!path.isValid()) || (path.isExistingDir() && (!path.isEmptyDir()))) {
      QMessageBox::critical(
          this, tr("Invalid Directory"),
          tr("The selected directory is invalid or not empty."));
      return false;
    } else
      return true;
  } else if (field("OpenWorkspace").toBool()) {
    FilePath path(field("OpenWorkspacePath").toString());
    if (!workspace::Workspace::isValidWorkspacePath(path)) {
      QMessageBox::critical(
          this, tr("Invalid Directory"),
          tr("The selected directory is not a valid workspace."));
      return false;
    } else
      return true;
  } else {
    Q_ASSERT(false);
    return false;
  }
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

void FirstRunWizardPage_WorkspacePath::on_rbtnCreateWs_toggled(bool checked) {
  mUi->lblCreateWs->setEnabled(checked);
  mUi->edtCreateWsPath->setEnabled(checked);
  mUi->btnCreateWsBrowse->setEnabled(checked);
  setFinalPage(!checked);  // force updating page order
}

void FirstRunWizardPage_WorkspacePath::on_rbtnOpenWs_toggled(bool checked) {
  mUi->lblOpenWs->setEnabled(checked);
  mUi->edtOpenWsPath->setEnabled(checked);
  mUi->btnOpenWsBrowse->setEnabled(checked);
  setFinalPage(checked);  // force updating page order
}

void FirstRunWizardPage_WorkspacePath::on_btnCreateWsBrowse_clicked() {
  QString filepath =
      FileDialog::getExistingDirectory(0, tr("Select Empty Directory"));
  if (filepath.isEmpty()) return;
  mUi->edtCreateWsPath->setText(filepath);
}

void FirstRunWizardPage_WorkspacePath::on_btnOpenWsBrowse_clicked() {
  QString filepath =
      FileDialog::getExistingDirectory(0, tr("Select Workspace Directory"));
  if (filepath.isEmpty()) return;
  mUi->edtOpenWsPath->setText(filepath);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
