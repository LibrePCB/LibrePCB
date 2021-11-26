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

#ifndef LIBREPCB_LIBRARY_EDITOR_EAGLELIBRARYIMPORTWIZARDCONTEXT_H
#define LIBREPCB_LIBRARY_EDITOR_EAGLELIBRARYIMPORTWIZARDCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>
#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace eagleimport {
class EagleLibraryImport;
}

namespace workspace {
class Workspace;
}

namespace library {
namespace editor {

/*******************************************************************************
 *  Class EagleLibraryImportWizardContext
 ******************************************************************************/

/**
 * @brief The EagleLibraryImportWizardContext class
 */
class EagleLibraryImportWizardContext final : public QObject {
  Q_OBJECT

public:
  static constexpr const char* NAME_PREFIX = "EAGLE_";

  // Constructors / Destructor
  EagleLibraryImportWizardContext() = delete;
  EagleLibraryImportWizardContext(
      const EagleLibraryImportWizardContext& other) = delete;
  EagleLibraryImportWizardContext(workspace::Workspace& workspace,
                                  const FilePath& dstLibFp,
                                  QObject* parent = nullptr) noexcept;
  ~EagleLibraryImportWizardContext() noexcept;

  // Getters
  workspace::Workspace& getWorkspace() const noexcept { return mWorkspace; }
  eagleimport::EagleLibraryImport& getImport() noexcept { return *mImport; }
  const FilePath& getLbrFilePath() const noexcept { return mLbrFilePath; }
  bool getAddNamePrefix() const noexcept { return mAddNamePrefix; }
  const tl::optional<Uuid>& getComponentCategory() const noexcept {
    return mComponentCategoryUuid;
  }
  const tl::optional<Uuid>& getPackageCategory() const noexcept {
    return mPackageCategoryUuid;
  }

  // Setters
  void setLbrFilePath(const QString& filePath) noexcept;
  void setAddNamePrefix(bool add) noexcept;
  void setComponentCategory(const tl::optional<Uuid>& uuid) noexcept;
  void setPackageCategory(const tl::optional<Uuid>& uuid) noexcept;

  // Operator Overloadings
  EagleLibraryImportWizardContext& operator=(
      const EagleLibraryImportWizardContext& rhs) = delete;

signals:
  void parseCompleted(const QString& messages);

private:  // Data
  workspace::Workspace& mWorkspace;
  QScopedPointer<eagleimport::EagleLibraryImport> mImport;
  FilePath mLbrFilePath;
  bool mAddNamePrefix;
  tl::optional<Uuid> mComponentCategoryUuid;
  tl::optional<Uuid> mPackageCategoryUuid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
