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
#include "kicadlibraryimportwizardpage_chooselibrary.h"

#include "../../dialogs/filedialog.h"
#include "../../editorcommandset.h"
#include "kicadlibraryimportwizardcontext.h"
#include "ui_kicadlibraryimportwizardpage_chooselibrary.h"

#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/kicadimport/kicadlibraryimport.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryImportWizardPage_ChooseLibrary::
    KiCadLibraryImportWizardPage_ChooseLibrary(
        std::shared_ptr<KiCadLibraryImportWizardContext> context,
        QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::KiCadLibraryImportWizardPage_ChooseLibrary),
    mContext(context),
    mLogger(new MessageLogger(false)) {
  mUi->setupUi(this);
  mUi->lblKiCadVersion->setText(mUi->lblKiCadVersion->text().arg("8.x"));
  mUi->edtDirPath->setText("-");  // Workaround to force initial library load.

  connect(mLogger.get(), &MessageLogger::msgEmitted, this,
          [this](const MessageLogger::Message& msg) {
            const QString txt = mUi->lblMessages->text();
            mUi->lblMessages->setText(txt + "\n" + msg.message);
          });
  connect(
      mUi->edtDirPath, &QLineEdit::textChanged, this,
      [this](const QString& filePath) {
        mUi->lblMessages->clear();
        qApp->setOverrideCursor(Qt::WaitCursor);
        mContext->setLibsPath(filePath, QString(), mLogger);
      },
      Qt::QueuedConnection);
  connect(
      mContext.get(), &KiCadLibraryImportWizardContext::scanFinished, this,
      [this]() {
        qApp->restoreOverrideCursor();
        emit completeChanged();
      },
      Qt::QueuedConnection);

  // Add browse action.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  QAction* aBrowse = cmd.inputBrowse.createAction(
      mUi->edtDirPath, this,
      [this]() {
        QString p = mUi->edtDirPath->text();
        if (p.trimmed().isEmpty()) {
          p = QDir::homePath();
        }
        p = FileDialog::getExistingDirectory(this, tr("Choose directory"), p);
        if (!p.isEmpty()) {
          mUi->edtDirPath->setText(p);
        }
      },
      EditorCommand::ActionFlag::WidgetShortcut);
  mUi->edtDirPath->addAction(aBrowse, QLineEdit::TrailingPosition);
}

KiCadLibraryImportWizardPage_ChooseLibrary::
    ~KiCadLibraryImportWizardPage_ChooseLibrary() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_ChooseLibrary::initializePage() {
  mUi->lblMessages->clear();
  mUi->edtDirPath->setText(mContext->getLibsPath().toNative());
  mContext->setLibsPath(mUi->edtDirPath->text(), QString(), mLogger);
}

bool KiCadLibraryImportWizardPage_ChooseLibrary::isComplete() const {
  return mContext->getImport().canStartParsing();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
