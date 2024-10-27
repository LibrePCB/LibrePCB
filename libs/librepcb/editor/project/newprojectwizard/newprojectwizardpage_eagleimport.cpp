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
#include "newprojectwizardpage_eagleimport.h"

#include "../../dialogs/filedialog.h"
#include "../../editorcommandset.h"
#include "../../utils/editortoolbox.h"
#include "../../widgets/waitingspinnerwidget.h"
#include "../../workspace/desktopservices.h"
#include "ui_newprojectwizardpage_eagleimport.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/eagleimport/eagleprojectimport.h>

#include <QtConcurrent>
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

NewProjectWizardPage_EagleImport::NewProjectWizardPage_EagleImport(
    const Workspace& ws, QWidget* parent) noexcept
  : QWizardPage(parent),
    mWorkspace(ws),
    mUi(new Ui::NewProjectWizardPage_EagleImport) {
  mUi->setupUi(this);
  setPixmap(QWizard::LogoPixmap, QPixmap(":/img/actions/plus_2.png"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));
  mWaitingSpinner.reset(new WaitingSpinnerWidget(mUi->sclMessages));
  mWaitingSpinner->hide();

  // Setup schematic input field.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  QAction* aBrowseSch = cmd.inputBrowse.createAction(
      mUi->edtSchematicFilePath, this,
      [this]() {
        const QString fp = FileDialog::getOpenFileName(
            this, tr("Select EAGLE Schematic"),
            mUi->edtSchematicFilePath->text(), "*.sch");
        if (!fp.isEmpty()) {
          mUi->edtSchematicFilePath->setText(fp);
          const QString brdFp = fp.chopped(4) % ".brd";
          if (FilePath(brdFp).isExistingFile()) {
            mUi->edtBoardFilePath->setText(brdFp);  // Import board too.
          } else {
            mUi->edtBoardFilePath->clear();  // Don't import any board.
          }
        }
      },
      EditorCommand::ActionFlag::WidgetShortcut);
  mUi->edtSchematicFilePath->addAction(aBrowseSch, QLineEdit::TrailingPosition);
  connect(mUi->edtSchematicFilePath, &QLineEdit::textChanged, this,
          &NewProjectWizardPage_EagleImport::updateStatus);

  // Setup board input field.
  QAction* aBrowseBrd = cmd.inputBrowse.createAction(
      mUi->edtBoardFilePath, this,
      [this]() {
        const QString fp =
            FileDialog::getOpenFileName(this, tr("Select EAGLE Board"),
                                        mUi->edtBoardFilePath->text(), "*.brd");
        if (!fp.isEmpty()) {
          mUi->edtBoardFilePath->setText(fp);
        }
      },
      EditorCommand::ActionFlag::WidgetShortcut);
  mUi->edtBoardFilePath->addAction(aBrowseBrd, QLineEdit::TrailingPosition);
  connect(mUi->edtBoardFilePath, &QLineEdit::textChanged, this,
          &NewProjectWizardPage_EagleImport::updateStatus);
  mUi->edtBoardFilePath->setEnabled(false);

  // Load settings.
  QSettings clientSettings;
  mUi->edtSchematicFilePath->setText(
      clientSettings.value("new_project_wizard/eagle_import/schematic_file")
          .toString());
  mUi->edtBoardFilePath->setText(
      clientSettings.value("new_project_wizard/eagle_import/board_file")
          .toString());

  // Periodically update status.
  QTimer* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this,
          &NewProjectWizardPage_EagleImport::updateStatus);
  timer->start(100);
}

NewProjectWizardPage_EagleImport::~NewProjectWizardPage_EagleImport() noexcept {
  // Save settings.
  QSettings clientSettings;
  clientSettings.setValue("new_project_wizard/eagle_import/schematic_file",
                          mUi->edtSchematicFilePath->text());
  clientSettings.setValue("new_project_wizard/eagle_import/board_file",
                          mUi->edtBoardFilePath->text());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NewProjectWizardPage_EagleImport::import(Project& project) {
  if (!mImport) {
    throw LogicError(__FILE__, __LINE__, "No importer available.");
  }

  // Displaying the messages in a dialog which remains open even after the
  // import finished. Unfortunately this is a bit hacky, some day it should be
  // improved.
  QDialog* dialog = new QDialog();
  dialog->setWindowTitle(tr("EAGLE Project Import"));
  dialog->setWindowIcon(QIcon(":/img/logo/64x64.png"));
  QVBoxLayout* layout = new QVBoxLayout(dialog);
  QTextBrowser* browser = new QTextBrowser(dialog);
  browser->setWordWrapMode(QTextOption::NoWrap);
  browser->setOpenLinks(false);
  const Workspace* ws = &mWorkspace;
  connect(browser, &QTextBrowser::anchorClicked, ws, [ws](const QUrl& url) {
    DesktopServices ds(ws->getSettings(), nullptr);
    ds.openUrl(url);
  });
  const auto msgColors = EditorToolbox::isWindowBackgroundDark()
      ? MessageLogger::ColorTheme::Dark
      : MessageLogger::ColorTheme::Light;
  connect(mImport->getLogger().get(), &MessageLogger::msgEmitted, browser,
          [browser, msgColors](const MessageLogger::Message& msg) {
            browser->append(msg.toRichText(msgColors, true));
            browser->verticalScrollBar()->setValue(
                browser->verticalScrollBar()->maximum());
            qApp->processEvents();
          });
  layout->addWidget(browser);
  QPushButton* btnClose = new QPushButton(tr("Close"), dialog);
  btnClose->setEnabled(false);
  connect(this, &NewProjectWizardPage_EagleImport::destroyed, btnClose,
          &QPushButton::setEnabled);
  connect(btnClose, &QPushButton::clicked, dialog, &QDialog::close);
  layout->addWidget(btnClose);
  dialog->resize(800, 600);
  dialog->show();
  qApp->processEvents();

  // Run the import (long-running blocking operation).
  mImport->import(project);  // can throw

  // After the project editor has been opened, bring messages dialog to front.
  // A bit hacky, probably some day we will need a better solution for this...
  QTimer::singleShot(2500, dialog, [dialog]() {
    dialog->raise();
    dialog->activateWindow();
  });
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewProjectWizardPage_EagleImport::updateStatus() noexcept {
  const QString sch = mUi->edtSchematicFilePath->text().trimmed();
  const QString brd = mUi->edtBoardFilePath->text().trimmed();
  const FilePath schFp(sch);
  const FilePath brdFp(brd);
  mUi->edtBoardFilePath->setEnabled(schFp.isValid());

  if (((sch != mCurrentSchematic) || (brd != mCurrentBoard)) &&
      (!mFuture.isRunning())) {
    mImport.reset();
    mCurrentSchematic = sch;
    mCurrentBoard = brd;
    if (sch.isEmpty()) {
      mUi->lblMessages->hide();
    } else if (schFp.isValid() && (brdFp.isValid() || brd.isEmpty())) {
      mWaitingSpinner->show();
      mUi->lblMessages->setText("<font color=\"blue\">" %
                                tr("Parsing project...") % "</font>");
      mUi->lblMessages->show();
      mFuture = QtConcurrent::run(
          &parseAsync, std::make_shared<eagleimport::EagleProjectImport>(),
          schFp, brdFp);
    } else {
      mUi->lblMessages->setText("<font color=\"red\">⚠ " %
                                tr("Invalid file path(s).") % "</font>");
      mUi->lblMessages->show();
    }
    emit completeChanged();
  }

  if (mFuture.isResultReadyAt(0)) {
    const auto result = mFuture.result();
    mFuture = {};
    mImport = result.import;
    mUi->lblMessages->setText(result.messages.join("<br>"));
    mUi->lblMessages->setVisible(!result.messages.isEmpty());
    mWaitingSpinner->hide();
    emit completeChanged();
  }
}

NewProjectWizardPage_EagleImport::ParserResult
    NewProjectWizardPage_EagleImport::parseAsync(
        std::shared_ptr<eagleimport::EagleProjectImport> import,
        const FilePath& schFp, const FilePath& brdFp) noexcept {
  ParserResult result;
  try {
    result.import = import;
    const QStringList warnings = result.import->open(schFp, brdFp);
    foreach (const QString& warning, warnings) {
      result.messages.append("<font color=\"blue\">➤ " % warning % "</font>");
    }
    const QString msg = result.import->hasBoard()
        ? tr("Ready to import %n sheet(s) and a board.", nullptr,
             result.import->getSheetCount())
        : tr("Ready to import %n sheet(s) without board.", nullptr,
             result.import->getSheetCount());
    result.messages.append("<font color=\"green\">✔ " % msg % "</font>");
  } catch (const Exception& e) {
    result.messages.append("<font color=\"red\">⚠ " % tr("ERROR:") % " " %
                           e.getMsg() % "</font>");
  }
  return result;
}

bool NewProjectWizardPage_EagleImport::isComplete() const noexcept {
  // Check base class.
  if (!QWizardPage::isComplete()) return false;

  // Check EAGLE project.
  if ((!mImport) || (!mImport->isReady())) return false;

  // Preselect project name for next wizard page.
  emit projectSelected(mImport->getProjectName());

  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
