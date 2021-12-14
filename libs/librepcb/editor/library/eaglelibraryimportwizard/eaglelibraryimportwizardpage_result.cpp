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
#include "eaglelibraryimportwizardpage_result.h"

#include "eaglelibraryimportwizardcontext.h"
#include "ui_eaglelibraryimportwizardpage_result.h"

#include <librepcb/eagleimport/eaglelibraryimport.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

using eagleimport::EagleLibraryImport;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryImportWizardPage_Result::EagleLibraryImportWizardPage_Result(
    std::shared_ptr<EagleLibraryImportWizardContext> context,
    QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::EagleLibraryImportWizardPage_Result),
    mContext(context),
    mIsCompleted(false) {
  mUi->setupUi(this);
  mUi->gbxErrors->hide();
  connect(&mContext->getImport(), &EagleLibraryImport::finished, this,
          &EagleLibraryImportWizardPage_Result::importFinished);

  // Connect finished signal directly with library scanner to get it emitted
  // even when closing this wizard while the import is still in progress.
  connect(&mContext->getImport(), &EagleLibraryImport::finished,
          &mContext->getWorkspace().getLibraryDb(),
          &workspace::WorkspaceLibraryDb::startLibraryRescan);
}

EagleLibraryImportWizardPage_Result::
    ~EagleLibraryImportWizardPage_Result() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EagleLibraryImportWizardPage_Result::initializePage() {
  while (!mProgressBarConnections.isEmpty()) {
    disconnect(mProgressBarConnections.takeLast());
  }
  mProgressBarConnections.append(
      connect(&mContext->getImport(), &EagleLibraryImport::progressStatus,
              mUi->prgImport, &QProgressBar::setFormat));
  mProgressBarConnections.append(
      connect(&mContext->getImport(), &EagleLibraryImport::progressPercent,
              mUi->prgImport, &QProgressBar::setValue));

  mIsCompleted = false;
  mUi->gbxErrors->hide();
  mUi->lblMessages->setText(QString());
  mUi->prgImport->setValue(0);
  mUi->prgImport->setFormat(QString());

  mContext->getImport().start();
}

bool EagleLibraryImportWizardPage_Result::isComplete() const {
  return mIsCompleted;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EagleLibraryImportWizardPage_Result::importFinished(
    const QStringList& errors) noexcept {
  while (!mProgressBarConnections.isEmpty()) {
    disconnect(mProgressBarConnections.takeLast());
  }
  mProgressBarConnections.append(
      connect(&mContext->getWorkspace().getLibraryDb(),
              &workspace::WorkspaceLibraryDb::scanProgressUpdate,
              mUi->prgImport, &QProgressBar::setValue, Qt::QueuedConnection));

  mUi->lblMessages->setText(errors.join("\n"));
  mUi->prgImport->setFormat(tr("Scanning libraries") % " (%p%)");
  mUi->gbxErrors->setVisible(!errors.isEmpty());
  if (QWizard* wiz = wizard()) {
    // Show restart button to allow importing a next library.
    wiz->setOption(QWizard::HaveCustomButton1, true);
  }
  mIsCompleted = true;
  emit completeChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
