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
#include "kicadlibraryimportwizardcontext.h"

#include <librepcb/core/exceptions.h>
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

KiCadLibraryImportWizardContext::KiCadLibraryImportWizardContext(
    Workspace& workspace, const FilePath& dstLibFp, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(workspace),
    mImport(new KiCadLibraryImport(dstLibFp, parent)),
    mLibsDirPath(),
    mComponentCategoryUuid(),
    mPackageCategoryUuid() {
  // Load settings.
  QSettings clientSettings;
  mLibsDirPath.setPath(
      clientSettings.value("library_editor/kicad_import_wizard/directory")
          .toString());
  setComponentCategory(Uuid::tryFromString(
      clientSettings
          .value("library_editor/kicad_import_wizard/component_category")
          .toString()));
  setPackageCategory(Uuid::tryFromString(
      clientSettings
          .value("library_editor/kicad_import_wizard/package_category")
          .toString()));
}

KiCadLibraryImportWizardContext::~KiCadLibraryImportWizardContext() noexcept {
  // Save settings.
  QSettings clientSettings;
  clientSettings.setValue("library_editor/kicad_import_wizard/directory",
                          mLibsDirPath.toStr());
  clientSettings.setValue(
      "library_editor/kicad_import_wizard/component_category",
      mComponentCategoryUuid ? mComponentCategoryUuid->toStr() : QString());
  clientSettings.setValue(
      "library_editor/kicad_import_wizard/package_category",
      mPackageCategoryUuid ? mPackageCategoryUuid->toStr() : QString());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void KiCadLibraryImportWizardContext::setLibsDirPath(
    const FilePath& fp) noexcept {
  if (fp != mLibsDirPath) {
    mLibsDirPath = fp;
    mLibraryData.reset();
    mImport->reset();
  }
}

void KiCadLibraryImportWizardContext::setComponentCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  // mComponentCategoryUuid = uuid;
  // mImport->setSymbolCategories(mComponentCategoryUuid
  //                                  ? QSet<Uuid>{*mComponentCategoryUuid}
  //                                  : QSet<Uuid>{});
  // mImport->setComponentCategories(mComponentCategoryUuid
  //                                     ? QSet<Uuid>{*mComponentCategoryUuid}
  //                                     : QSet<Uuid>{});
  // mImport->setDeviceCategories(mComponentCategoryUuid
  //                                  ? QSet<Uuid>{*mComponentCategoryUuid}
  //                                  : QSet<Uuid>{});
}

void KiCadLibraryImportWizardContext::setPackageCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  // mPackageCategoryUuid = uuid;
  // mImport->setPackageCategories(
  //     mPackageCategoryUuid ? QSet<Uuid>{*mPackageCategoryUuid} :
  //     QSet<Uuid>{});
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
