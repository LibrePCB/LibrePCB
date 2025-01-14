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
#include "kicadlibraryimportwizardpage_result.h"

#include "../../utils/editortoolbox.h"
#include "../../workspace/desktopservices.h"
#include "kicadlibraryimportwizardcontext.h"
#include "ui_kicadlibraryimportwizardpage_result.h"

#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/kicadimport/kicadlibraryimport.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using kicadimport::KiCadLibraryImport;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryImportWizardPage_Result::KiCadLibraryImportWizardPage_Result(
    std::shared_ptr<KiCadLibraryImportWizardContext> context,
    QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::KiCadLibraryImportWizardPage_Result),
    mContext(context),
    mIsCompleted(false) {
  mUi->setupUi(this);
  connect(mUi->txtMessages, &QTextBrowser::anchorClicked, this,
          [this](const QUrl& url) {
            DesktopServices ds(mContext->getWorkspace().getSettings());
            ds.openWebUrl(url);
          });
  connect(&mContext->getImport(), &KiCadLibraryImport::importFinished, this,
          &KiCadLibraryImportWizardPage_Result::importFinished);

  // Connect finished signal directly with library scanner to get it emitted
  // even when closing this wizard while the import is still in progress.
  connect(&mContext->getImport(), &KiCadLibraryImport::importFinished,
          &mContext->getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::startLibraryRescan);
}

KiCadLibraryImportWizardPage_Result::
    ~KiCadLibraryImportWizardPage_Result() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_Result::initializePage() {
  while (!mProgressBarConnections.isEmpty()) {
    disconnect(mProgressBarConnections.takeLast());
  }
  mProgressBarConnections.append(
      connect(&mContext->getImport(), &KiCadLibraryImport::progressStatus,
              mUi->prgImport, &QProgressBar::setFormat));
  mProgressBarConnections.append(
      connect(&mContext->getImport(), &KiCadLibraryImport::progressPercent,
              mUi->prgImport, &QProgressBar::setValue));

  mIsCompleted = false;
  mUi->txtMessages->clear();
  mUi->prgImport->setValue(0);
  mUi->prgImport->setFormat(QString());
  mUi->prgImport->show();
  if (QWizard* wiz = wizard()) {
    // Show cancel button during import.
    wiz->setOption(QWizard::NoCancelButtonOnLastPage, false);
  }

  const MessageLogger::ColorTheme msgColors =
      EditorToolbox::isWindowBackgroundDark()
      ? MessageLogger::ColorTheme::Dark
      : MessageLogger::ColorTheme::Light;
  auto log = std::make_shared<MessageLogger>();
  connect(&mContext->getImport(), &KiCadLibraryImport::importFinished, this,
          [log]() {
            log->info(QString());
            log->info(
                tr("It is highly recommended to review and rework the imported "
                   "elements:"));
            log->info(" • " % tr("Assign reasonable categories"));
            log->info(" • " % tr("Review/correct pinouts of devices"));
            log->info(" • " %
                      tr("Review/rework geometry of symbols and footprints"));
            log->info(" • " %
                      tr("Fix remaining warnings shown in the library editor"));
          });
  connect(log.get(), &MessageLogger::msgEmitted, this,
          [this, msgColors](const MessageLogger::Message& msg) {
            mUi->txtMessages->append(msg.toRichText(msgColors));
            mUi->txtMessages->verticalScrollBar()->setValue(
                mUi->txtMessages->verticalScrollBar()->maximum());
          });
  mContext->getImport().startImport(log);
}

bool KiCadLibraryImportWizardPage_Result::isComplete() const {
  return mIsCompleted;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_Result::importFinished() noexcept {
  while (!mProgressBarConnections.isEmpty()) {
    disconnect(mProgressBarConnections.takeLast());
  }
  mProgressBarConnections.append(
      connect(&mContext->getWorkspace().getLibraryDb(),
              &WorkspaceLibraryDb::scanProgressUpdate, mUi->prgImport,
              &QProgressBar::setValue, Qt::QueuedConnection));
  mProgressBarConnections.append(
      connect(&mContext->getWorkspace().getLibraryDb(),
              &WorkspaceLibraryDb::scanFinished, mUi->prgImport,
              &QProgressBar::hide, Qt::QueuedConnection));

  mUi->prgImport->setFormat(tr("Scanning libraries") % " (%p%)");
  if (QWizard* wiz = wizard()) {
    // Hide Cancel button.
    wiz->setOption(QWizard::NoCancelButtonOnLastPage, true);
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
}  // namespace librepcb
