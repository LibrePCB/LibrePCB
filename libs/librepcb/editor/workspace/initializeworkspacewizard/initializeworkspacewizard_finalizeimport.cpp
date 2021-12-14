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
#include "initializeworkspacewizard_finalizeimport.h"

#include "ui_initializeworkspacewizard_finalizeimport.h"

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

InitializeWorkspaceWizard_FinalizeImport::
    InitializeWorkspaceWizard_FinalizeImport(
        InitializeWorkspaceWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::InitializeWorkspaceWizard_FinalizeImport),
    mCopyOperation(),
    mImportSucceeded(false) {
  mUi->setupUi(this);
  mUi->progressBar->hide();
  connect(mUi->pushButton, &QPushButton::clicked, this,
          &InitializeWorkspaceWizard_FinalizeImport::startImport);
}

InitializeWorkspaceWizard_FinalizeImport::
    ~InitializeWorkspaceWizard_FinalizeImport() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void InitializeWorkspaceWizard_FinalizeImport::initializePage() noexcept {
  mCopyOperation = mContext.createImportCopyOperation();
  if (mCopyOperation) {
    mUi->lblSourceDir->setText(mCopyOperation->getSource().toNative());
    mUi->lblDestinationDir->setText(
        mCopyOperation->getDestination().toNative());
    connect(mCopyOperation.get(), &AsyncCopyOperation::progressStatus,
            mUi->lblStatus, &QLabel::setText);
    connect(mCopyOperation.get(), &AsyncCopyOperation::progressPercent,
            mUi->progressBar, &QProgressBar::setValue);
    connect(mCopyOperation.get(), &AsyncCopyOperation::succeeded, this,
            &InitializeWorkspaceWizard_FinalizeImport::importSucceeded);
  }
}

bool InitializeWorkspaceWizard_FinalizeImport::isComplete() const noexcept {
  return mImportSucceeded;
}

int InitializeWorkspaceWizard_FinalizeImport::nextId() const noexcept {
  return InitializeWorkspaceWizardContext::ID_None;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void InitializeWorkspaceWizard_FinalizeImport::startImport() noexcept {
  if (mCopyOperation) {
    mUi->progressBar->show();
    mUi->pushButton->hide();
    mCopyOperation->start();
  }
}

void InitializeWorkspaceWizard_FinalizeImport::importFailed(
    const QString& error) noexcept {
  Q_UNUSED(error);
  mUi->pushButton->show();
}

void InitializeWorkspaceWizard_FinalizeImport::importSucceeded() noexcept {
  mImportSucceeded = true;
  emit completeChanged();

  // Disable the "cancel" and "back" buttons since they do not make any sense
  // after the import was completed. The only remaining button is "finish".
  // In addition, it fixes https://github.com/LibrePCB/LibrePCB/issues/675.
  if (QAbstractButton* btn = wizard()->button(QWizard::BackButton)) {
    btn->setEnabled(false);
  }
  if (QAbstractButton* btn = wizard()->button(QWizard::CancelButton)) {
    btn->setEnabled(false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
