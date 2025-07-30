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
#include "kicadlibraryimportwizard.h"

#include "kicadlibraryimportwizardcontext.h"
#include "kicadlibraryimportwizardpage_chooselibrary.h"
#include "kicadlibraryimportwizardpage_parse.h"
#include "kicadlibraryimportwizardpage_result.h"
#include "kicadlibraryimportwizardpage_selectelements.h"
#include "kicadlibraryimportwizardpage_setoptions.h"
#include "kicadlibraryimportwizardpage_start.h"
#include "ui_kicadlibraryimportwizard.h"

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

KiCadLibraryImportWizard::KiCadLibraryImportWizard(Workspace& workspace,
                                                   const FilePath& dstLibFp,
                                                   QWidget* parent) noexcept
  : QWizard(parent),
    mUi(new Ui::KiCadLibraryImportWizard),
    mContext(new KiCadLibraryImportWizardContext(workspace, dstLibFp, this)) {
  mUi->setupUi(this);
  setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/img/logo/48x48.png"));
  setPixmap(QWizard::WatermarkPixmap,
            QPixmap(":/img/wizards/kicad_watermark.png"));

  // Add pages.
  addPage(new KiCadLibraryImportWizardPage_Start(mContext, this));
  addPage(new KiCadLibraryImportWizardPage_ChooseLibrary(mContext, this));
  addPage(new KiCadLibraryImportWizardPage_Parse(mContext, this));
  addPage(new KiCadLibraryImportWizardPage_SelectElements(mContext, this));
  addPage(new KiCadLibraryImportWizardPage_SetOptions(mContext, this));
  addPage(new KiCadLibraryImportWizardPage_Result(mContext, this));

  // Add restart button to allow importing a next library without closing the
  // wizard. The button will be enabled in the last page, and removed when
  // clicking on it.
  setButtonText(QWizard::CustomButton1, tr("&Restart"));
  connect(
      this, &QWizard::customButtonClicked, this,
      [this]() {
        // Hide restart button and start over.
        setOption(QWizard::HaveCustomButton1, false);
        mContext->getImport().reset();
        restart();
      },
      Qt::QueuedConnection);

  // Load window geometry.
  QSettings cs;
  const QSize windowSize = cs.value("kicad_import_wizard/window_size").toSize();
  if (!windowSize.isEmpty()) {
    resize(windowSize);
  }
}

KiCadLibraryImportWizard::~KiCadLibraryImportWizard() noexcept {
  // Save window geometry.
  QSettings cs;
  cs.setValue("kicad_import_wizard/window_size", size());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImportWizard::reject() noexcept {
  if (mContext->getImport().isRunning()) {
    int result = QMessageBox::warning(
        this, tr("Abort"),
        tr("An operation is currently in progress. Closing this window will "
           "abort it.\n\nDo you really want to close it?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result != QMessageBox::Yes) {
      return;  // Abort, do not close the wizard.
    }
  }

  QWizard::reject();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
