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
#include "../../editorcommandset.h"
#include "ui_initializeworkspacewizard_chooseworkspace.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>

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
  connect(mUi->edtPath, &QLineEdit::textChanged, this,
          &InitializeWorkspaceWizard_ChooseWorkspace::updateWorkspacePath);

  // Add browse action.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  QAction* aBrowse = cmd.inputBrowse.createAction(
      mUi->edtPath, this,
      [this]() {
        QString filepath = FileDialog::getExistingDirectory(
            this, tr("Select Workspace Directory"));
        if (!filepath.isEmpty()) {
          mUi->edtPath->setText(filepath);
        }
      },
      EditorCommand::ActionFlag::WidgetShortcut);
  mUi->edtPath->addAction(aBrowse, QLineEdit::TrailingPosition);
}

InitializeWorkspaceWizard_ChooseWorkspace::
    ~InitializeWorkspaceWizard_ChooseWorkspace() noexcept {
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

void InitializeWorkspaceWizard_ChooseWorkspace::initializePage() noexcept {
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

  FilePath fp = mContext.getWorkspacePath();
  if (!fp.isValid()) {
    fp = defaultWsPath;
  }
  mUi->edtPath->setText(fp.toNative());
  mUi->edtPath->selectAll();
  mUi->edtPath->setFocus();
}

bool InitializeWorkspaceWizard_ChooseWorkspace::isComplete() const noexcept {
  return mContext.isWorkspacePathValid();
}

int InitializeWorkspaceWizard_ChooseWorkspace::nextId() const noexcept {
  if (mContext.getNeedsUpgrade()) {
    return InitializeWorkspaceWizardContext::ID_Upgrade;
  } else if (mContext.getNeedsInitialization()) {
    return InitializeWorkspaceWizardContext::ID_ChooseSettings;
  } else {
    return InitializeWorkspaceWizardContext::ID_None;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void InitializeWorkspaceWizard_ChooseWorkspace::updateWorkspacePath() noexcept {
  QString message;

  try {
    FilePath path(mUi->edtPath->text());
    mContext.setWorkspacePath(path);  // can throw
    if (!path.isValid()) {
      message = tr("Please select a directory.");
    } else if (mContext.getWorkspaceExists()) {
      message = tr("Directory contains a valid workspace.");
    } else if (mContext.isWorkspacePathValid()) {
      message = tr("New workspace will be created.");
    } else {
      message = tr("Directory is not empty!");
    }
  } catch (const Exception& e) {
    message = e.getMsg();
  }

  if (mContext.getWorkspaceExists()) {
    mUi->lblStatus->setText("<font color=\"green\">✔ " % message % "</font>");
  } else if (mContext.isWorkspacePathValid()) {
    mUi->lblStatus->setText("<font color=\"blue\">➤ " % message % "</font>");
  } else {
    mUi->lblStatus->setText("<font color=\"red\">⚠ " % message % "</font>");
  }

  // Workaround to force nextId() to be reloaded.
  setFinalPage(true);
  setFinalPage(false);

  emit completeChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
