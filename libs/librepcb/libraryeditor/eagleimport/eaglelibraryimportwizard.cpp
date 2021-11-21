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
#include "eaglelibraryimportwizard.h"

#include "eaglelibraryimportwizardcontext.h"
#include "eaglelibraryimportwizardpage_chooselibrary.h"
#include "eaglelibraryimportwizardpage_result.h"
#include "eaglelibraryimportwizardpage_selectelements.h"
#include "eaglelibraryimportwizardpage_setoptions.h"
#include "eaglelibraryimportwizardpage_start.h"
#include "ui_eaglelibraryimportwizard.h"

#include <librepcb/eagleimport/eaglelibraryimport.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryImportWizard::EagleLibraryImportWizard(
    workspace::Workspace& workspace, const FilePath& dstLibFp,
    QWidget* parent) noexcept
  : QWizard(parent),
    mUi(new Ui::EagleLibraryImportWizard),
    mContext(new EagleLibraryImportWizardContext(workspace, dstLibFp, this)) {
  mUi->setupUi(this);
  setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/img/logo/48x48.png"));
  setPixmap(QWizard::WatermarkPixmap,
            QPixmap(":/img/wizards/eagle_watermark.png"));

  // Add pages.
  addPage(new EagleLibraryImportWizardPage_Start(mContext, this));
  addPage(new EagleLibraryImportWizardPage_ChooseLibrary(mContext, this));
  addPage(new EagleLibraryImportWizardPage_SelectElements(mContext, this));
  addPage(new EagleLibraryImportWizardPage_SetOptions(mContext, this));
  addPage(new EagleLibraryImportWizardPage_Result(mContext, this));

  // Add restart button to allow importing a next library without closing the
  // wizard. The button will be enabled in the last page, and removed when
  // clicking on it.
  setButtonText(QWizard::CustomButton1, tr("&Restart"));
  connect(this, &QWizard::customButtonClicked, this,
          [this]() {
            // Hide restart button and start over.
            setOption(QWizard::HaveCustomButton1, false);
            restart();
            next();
          },
          Qt::QueuedConnection);

  // Load window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("library_editor/eagle_import_wizard/window_geometry")
          .toByteArray());
}

EagleLibraryImportWizard::~EagleLibraryImportWizard() noexcept {
  // Save window geometry.
  QSettings clientSettings;
  clientSettings.setValue("library_editor/eagle_import_wizard/window_geometry",
                          saveGeometry());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EagleLibraryImportWizard::reject() noexcept {
  if (mContext->getImport().isRunning()) {
    int result = QMessageBox::warning(
        this, tr("Abort"),
        tr("The import is currently in progress. Closing this window will "
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
}  // namespace library
}  // namespace librepcb
