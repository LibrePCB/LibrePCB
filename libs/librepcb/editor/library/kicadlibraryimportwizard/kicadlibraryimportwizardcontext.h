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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDCONTEXT_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>
#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace kicadimport {
class KiCadLibraryImport;
}

namespace editor {

/*******************************************************************************
 *  Class KiCadLibraryImportWizardContext
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizardContext class
 */
class KiCadLibraryImportWizardContext final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryImportWizardContext() = delete;
  KiCadLibraryImportWizardContext(
      const KiCadLibraryImportWizardContext& other) = delete;
  KiCadLibraryImportWizardContext(Workspace& workspace,
                                  const FilePath& dstLibFp,
                                  QObject* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizardContext() noexcept;

  // Getters
  Workspace& getWorkspace() const noexcept { return mWorkspace; }
  // kicadimport::KiCadLibraryImport& getImport() noexcept { return *mImport; }
  const FilePath& getLibsDirPath() const noexcept { return mLibsDirPath; }
  const tl::optional<Uuid>& getComponentCategory() const noexcept {
    return mComponentCategoryUuid;
  }
  const tl::optional<Uuid>& getPackageCategory() const noexcept {
    return mPackageCategoryUuid;
  }

  // Setters
  void setLibsDirPath(const QString& filePath) noexcept;
  void setComponentCategory(const tl::optional<Uuid>& uuid) noexcept;
  void setPackageCategory(const tl::optional<Uuid>& uuid) noexcept;

  // Operator Overloadings
  KiCadLibraryImportWizardContext& operator=(
      const KiCadLibraryImportWizardContext& rhs) = delete;

signals:
  void parseCompleted(const QString& messages);

private:  // Data
  Workspace& mWorkspace;
  QScopedPointer<kicadimport::KiCadLibraryImport> mImport;
  FilePath mLibsDirPath;
  tl::optional<Uuid> mComponentCategoryUuid;
  tl::optional<Uuid> mPackageCategoryUuid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
