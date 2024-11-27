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
#include "kicadlibraryimportwizardpage_parse.h"

#include "../../utils/editortoolbox.h"
#include "kicadlibraryimportwizardcontext.h"
#include "ui_kicadlibraryimportwizardpage_parse.h"

#include <librepcb/core/utils/messagelogger.h>
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

KiCadLibraryImportWizardPage_Parse::KiCadLibraryImportWizardPage_Parse(
    std::shared_ptr<KiCadLibraryImportWizardContext> context,
    QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::KiCadLibraryImportWizardPage_Parse),
    mContext(context) {
  mUi->setupUi(this);
  connect(&mContext->getImport(), &KiCadLibraryImport::progressPercent,
          mUi->progressBar, &QProgressBar::setValue);
  connect(&mContext->getImport(), &KiCadLibraryImport::parseFinished,
          mUi->progressBar, &QProgressBar::hide);
  connect(&mContext->getImport(), &KiCadLibraryImport::parseFinished, this,
          &KiCadLibraryImportWizardPage_Parse::completeChanged,
          Qt::QueuedConnection);
}

KiCadLibraryImportWizardPage_Parse::
    ~KiCadLibraryImportWizardPage_Parse() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_Parse::initializePage() {
  if (mContext->getImport().getState() >= KiCadLibraryImport::State::Parsed) {
    return;  // Already parsed.
  }

  mUi->txtMessages->clear();
  mUi->txtMessages->show();
  mUi->progressBar->setValue(0);
  mUi->progressBar->show();

  std::shared_ptr<MessageLogger> log = std::make_shared<MessageLogger>(false);
  const MessageLogger::ColorTheme msgColors =
      EditorToolbox::isWindowBackgroundDark()
      ? MessageLogger::ColorTheme::Dark
      : MessageLogger::ColorTheme::Light;
  connect(log.get(), &MessageLogger::msgEmitted, this,
          [this, msgColors](const MessageLogger::Message& msg) {
            mUi->txtMessages->append(msg.toRichText(msgColors));
            mUi->txtMessages->verticalScrollBar()->setValue(
                mUi->txtMessages->verticalScrollBar()->maximum());
          });
  mContext->getImport().startParse(log);
}

bool KiCadLibraryImportWizardPage_Parse::isComplete() const {
  return mContext->getImport().canStartSelecting();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
