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
#include <librepcb/core/workspace/workspace.h>
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
    mLibsPath(),
    mAddNamePrefix(false),
    mComponentCategoryUuid(),
    mPackageCategoryUuid(),
    mImport(
        new KiCadLibraryImport(workspace.getLibraryDb(), dstLibFp, parent)) {
  connect(mImport.get(), &KiCadLibraryImport::scanFinished, this,
          &KiCadLibraryImportWizardContext::scanFinished, Qt::QueuedConnection);

  // Load settings.
  QSettings cs;
  mLibsPath.setPath(
      cs.value("library_editor/kicad_import_wizard/libs_path").toString());
  mShapes3dPath.setPath(
      cs.value("library_editor/kicad_import_wizard/shapes3d_path").toString());
  setAddNamePrefix(
      cs.value("library_editor/kicad_import_wizard/add_name_prefix", false)
          .toBool());
  setComponentCategory(Uuid::tryFromString(
      cs.value("library_editor/kicad_import_wizard/component_category")
          .toString()));
  setPackageCategory(Uuid::tryFromString(
      cs.value("library_editor/kicad_import_wizard/package_category")
          .toString()));
}

KiCadLibraryImportWizardContext::~KiCadLibraryImportWizardContext() noexcept {
  // Save settings.
  QSettings cs;
  cs.setValue("library_editor/kicad_import_wizard/libs_path",
              mLibsPath.toStr());
  cs.setValue("library_editor/kicad_import_wizard/shapes3d_path",
              mShapes3dPath.toStr());
  cs.setValue("library_editor/kicad_import_wizard/add_name_prefix",
              mAddNamePrefix);
  cs.setValue(
      "library_editor/kicad_import_wizard/component_category",
      mComponentCategoryUuid ? mComponentCategoryUuid->toStr() : QString());
  cs.setValue("library_editor/kicad_import_wizard/package_category",
              mPackageCategoryUuid ? mPackageCategoryUuid->toStr() : QString());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void KiCadLibraryImportWizardContext::setLibsPath(
    const QString& libsPath, const QString& shapes3dPath,
    std::shared_ptr<MessageLogger> log) noexcept {
  const FilePath libsFp(libsPath);
  const FilePath shapes3dFp(shapes3dPath);
  if (!libsFp.isValid()) {
    mImport->reset();
    log->info(tr("No file or directory selected."));
    emit scanFinished();
  } else if ((libsFp != mImport->getLoadedLibsPath()) ||
             (shapes3dFp != mImport->getLoadedShapes3dPath())) {
    mLibsPath = libsFp;
    mShapes3dPath = shapes3dFp;
    mImport->reset();
    mImport->startScan(libsFp, shapes3dFp, log);
  }
}

void KiCadLibraryImportWizardContext::setAddNamePrefix(bool add) noexcept {
  mAddNamePrefix = add;
  mImport->setNamePrefix(add ? NAME_PREFIX : QString());
}

void KiCadLibraryImportWizardContext::setComponentCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  mComponentCategoryUuid = uuid;
  mImport->setSymbolCategories(mComponentCategoryUuid
                                   ? QSet<Uuid>{*mComponentCategoryUuid}
                                   : QSet<Uuid>{});
  mImport->setComponentCategories(mComponentCategoryUuid
                                      ? QSet<Uuid>{*mComponentCategoryUuid}
                                      : QSet<Uuid>{});
  mImport->setDeviceCategories(mComponentCategoryUuid
                                   ? QSet<Uuid>{*mComponentCategoryUuid}
                                   : QSet<Uuid>{});
}

void KiCadLibraryImportWizardContext::setPackageCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  mPackageCategoryUuid = uuid;
  mImport->setPackageCategories(
      mPackageCategoryUuid ? QSet<Uuid>{*mPackageCategoryUuid} : QSet<Uuid>{});
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
