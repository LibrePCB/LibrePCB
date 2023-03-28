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
#include "initializeworkspacewizard_upgrade.h"

#include "ui_initializeworkspacewizard_upgrade.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/asynccopyoperation.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizard_Upgrade::InitializeWorkspaceWizard_Upgrade(
    InitializeWorkspaceWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::InitializeWorkspaceWizard_Upgrade),
    mCopyOperation() {
  mUi->setupUi(this);
  mUi->lblTitle->setText(
      tr("Upgrade to LibrePCB %1")
          .arg(Application::getFileFormatVersion().toStr() % ".x"));
  mUi->progressBarWidget->setMinimumHeight(
      mUi->progressBar->sizeHint().height());
  setButtonText(QWizard::FinishButton, tr("Upgrade"));
}

InitializeWorkspaceWizard_Upgrade::
    ~InitializeWorkspaceWizard_Upgrade() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void InitializeWorkspaceWizard_Upgrade::initializePage() noexcept {
  mUi->lblError->hide();
  mUi->progressBar->hide();
  mUi->lblInfo->show();

  const FilePath src = mContext.getWorkspacePath().getPathTo(
      mContext.getUpgradeCopyDirs().first);
  const FilePath dst = mContext.getWorkspacePath().getPathTo(
      mContext.getUpgradeCopyDirs().second);
  mUi->lblSource->setText(QString("<a href=\"%1\">%2</a>")
                              .arg(src.toQUrl().toString(), src.toNative()));
  mUi->lblDestination->setText(dst.toNative());

  mCopyOperation.reset(new AsyncCopyOperation(src, dst));
  connect(mCopyOperation.data(), &AsyncCopyOperation::progressPercent,
          mUi->progressBar, &QProgressBar::setValue);
  connect(mCopyOperation.data(), &AsyncCopyOperation::progressStatus,
          mUi->progressBar, &QProgressBar::setFormat);
  connect(mCopyOperation.data(), &AsyncCopyOperation::failed, this,
          [this](const QString& errorMsg) {
            mUi->lblInfo->hide();
            mUi->progressBar->hide();
            QString text =
                "<p><b>" % tr("Error:") % " " % errorMsg % "</b></p>";
            text += "<p>" %
                tr("If the error persists, you could try to copy the mentioned "
                   "directory manually (e.g. with your file manager).") %
                "</p>";
            mUi->lblError->setText(text);
            mUi->lblError->show();
          });
  connect(mCopyOperation.data(), &AsyncCopyOperation::succeeded, this,
          [this]() { QTimer::singleShot(700, wizard(), &QWizard::accept); });
  emit completeChanged();
}

bool InitializeWorkspaceWizard_Upgrade::validatePage() noexcept {
  if (!mCopyOperation) {
    return false;
  }

  if (mCopyOperation->isRunning() || mCopyOperation->isFinished()) {
    return true;
  }

  if (QAbstractButton* btn = wizard()->button(QWizard::BackButton)) {
    btn->setEnabled(false);
  } else {
    qWarning() << "Could not disable back button in workspace upgrade wizard.";
  }
  if (QAbstractButton* btn = wizard()->button(QWizard::FinishButton)) {
    btn->setEnabled(false);
  } else {
    qWarning()
        << "Could not disable finish button in workspace upgrade wizard.";
  }
  mUi->progressBar->show();
  mCopyOperation->start();
  return false;
}

int InitializeWorkspaceWizard_Upgrade::nextId() const noexcept {
  return InitializeWorkspaceWizardContext::ID_None;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
