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

#ifndef LIBREPCB_APPLICATION_INITIALIZEWORKSPACEWIZARDCONTEXT_H
#define LIBREPCB_APPLICATION_INITIALIZEWORKSPACEWIZARDCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/units/lengthunit.h>
#include <librepcb/common/version.h>
#include <optional/tl/optional.hpp>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AsyncCopyOperation;

namespace application {

/*******************************************************************************
 *  Class InitializeWorkspaceWizardContext
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizardContext class
 */
class InitializeWorkspaceWizardContext final : public QObject {
  Q_OBJECT

public:
  // Types

  enum PageId {
    ID_None = -1,  ///< last page
    ID_ChooseImportVersion,
    ID_FinalizeImport,
    ID_ChooseSettings,
  };

  // Constructors / Destructor
  InitializeWorkspaceWizardContext() = delete;
  InitializeWorkspaceWizardContext(
      const InitializeWorkspaceWizardContext& other) = delete;
  InitializeWorkspaceWizardContext(const FilePath& ws,
                                   QObject* parent = nullptr) noexcept;
  ~InitializeWorkspaceWizardContext() noexcept;

  // Getters
  const FilePath& getWorkspacePath() const noexcept { return mWorkspacePath; }
  const tl::optional<Version>& getVersionToImport() const noexcept {
    return mVersionToImport;
  }
  const QString& getAppLocale() const noexcept { return mAppLocale; }
  const LengthUnit& getLengthUnit() const noexcept { return mLengthUnit; }
  const QStringList& getLibraryNormOrder() const noexcept {
    return mLibraryNormOrder;
  }
  const QString& getUserName() const noexcept { return mUserName; }

  // Setters
  void setVersionToImport(const tl::optional<Version>& version) noexcept {
    mVersionToImport = version;
  }
  void setAppLocale(const QString& locale) noexcept { mAppLocale = locale; }
  void setLengthUnit(const LengthUnit& unit) noexcept { mLengthUnit = unit; }
  void setLibraryNormOrder(const QStringList& order) noexcept {
    mLibraryNormOrder = order;
  }
  void setUserName(const QString& name) noexcept { mUserName = name; }

  // General Methods
  std::unique_ptr<AsyncCopyOperation> createImportCopyOperation() const
      noexcept;
  void initializeEmptyWorkspace() const;

  // Operator Overloadings
  InitializeWorkspaceWizardContext& operator=(
      const InitializeWorkspaceWizardContext& rhs) = delete;

private:  // Data
  FilePath mWorkspacePath;
  tl::optional<Version> mVersionToImport;
  QString mAppLocale;
  LengthUnit mLengthUnit;
  QStringList mLibraryNormOrder;
  QString mUserName;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif  // LIBREPCB_APPLICATION_INITIALIZEWORKSPACEWIZARDCONTEXT_H
