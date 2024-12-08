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
#include "kicadlibraryimportwizardpage_setoptions.h"

#include "../cat/categorychooserdialog.h"
#include "../cat/categorytreelabeltextbuilder.h"
#include "kicadlibraryimportwizardcontext.h"
#include "ui_kicadlibraryimportwizardpage_setoptions.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>
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

KiCadLibraryImportWizardPage_SetOptions::
    KiCadLibraryImportWizardPage_SetOptions(
        std::shared_ptr<KiCadLibraryImportWizardContext> context,
        QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::KiCadLibraryImportWizardPage_SetOptions),
    mContext(context) {
  mUi->setupUi(this);
  mUi->cbxAddNamePrefix->setText(mUi->cbxAddNamePrefix->text().arg(
      KiCadLibraryImportWizardContext::NAME_PREFIX));
  setButtonText(QWizard::CommitButton, tr("&Import!"));
  setCommitPage(true);
  connect(mUi->cbxAddNamePrefix, &QCheckBox::toggled, mContext.get(),
          &KiCadLibraryImportWizardContext::setAddNamePrefix);
  connect(mUi->btnChooseComponentCategory, &QToolButton::clicked, this,
          [this]() {
            CategoryChooserDialog dialog(mContext->getWorkspace(),
                                         CategoryChooserDialog::Filter::CmpCat,
                                         this);
            if (dialog.exec() == QDialog::Accepted) {
              mContext->setComponentCategory(dialog.getSelectedCategoryUuid());
              updateComponentCategoryTreeLabel();
            }
          });
  connect(mUi->btnResetComponentCategory, &QToolButton::clicked, this,
          [this]() {
            mContext->setComponentCategory(std::nullopt);
            updateComponentCategoryTreeLabel();
          });
  connect(mUi->btnChoosePackageCategory, &QToolButton::clicked, this, [this]() {
    CategoryChooserDialog dialog(mContext->getWorkspace(),
                                 CategoryChooserDialog::Filter::PkgCat, this);
    if (dialog.exec() == QDialog::Accepted) {
      mContext->setPackageCategory(dialog.getSelectedCategoryUuid());
      updatePackageCategoryTreeLabel();
    }
  });
  connect(mUi->btnResetPackageCategory, &QToolButton::clicked, this, [this]() {
    mContext->setPackageCategory(std::nullopt);
    updatePackageCategoryTreeLabel();
  });
}

KiCadLibraryImportWizardPage_SetOptions::
    ~KiCadLibraryImportWizardPage_SetOptions() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_SetOptions::initializePage() {
  auto result = mContext->getImport().getResult();
  mUi->cbxAddNamePrefix->setChecked(mContext->getAddNamePrefix());
  mUi->gbxComponentCategory->setVisible(result &&
                                        (result->symbolLibs.count() > 0));
  mUi->gbxPackageCategory->setVisible(result &&
                                      (result->footprintLibs.count() > 0));
  updateComponentCategoryTreeLabel();
  updatePackageCategoryTreeLabel();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_SetOptions::
    updateComponentCategoryTreeLabel() noexcept {
  ComponentCategoryTreeLabelTextBuilder builder(
      mContext->getWorkspace().getLibraryDb(),
      mContext->getWorkspace().getSettings().libraryLocaleOrder.get(), false,
      *mUi->lblComponentCategoryTree);
  builder.setOneLine(true);
  builder.updateText(mContext->getComponentCategory());
}

void KiCadLibraryImportWizardPage_SetOptions::
    updatePackageCategoryTreeLabel() noexcept {
  PackageCategoryTreeLabelTextBuilder builder(
      mContext->getWorkspace().getLibraryDb(),
      mContext->getWorkspace().getSettings().libraryLocaleOrder.get(), false,
      *mUi->lblPackageCategoryTree);
  builder.setOneLine(true);
  builder.updateText(mContext->getPackageCategory());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
