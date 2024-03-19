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

#ifndef LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARDCONTEXT_H
#define LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARDCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

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
    ID_Welcome,
    ID_ChooseWorkspace,
    ID_Upgrade,
    ID_ChooseSettings,
  };

  // Constructors / Destructor
  InitializeWorkspaceWizardContext(
      const InitializeWorkspaceWizardContext& other) = delete;
  explicit InitializeWorkspaceWizardContext(QObject* parent = nullptr) noexcept;
  ~InitializeWorkspaceWizardContext() noexcept;

  // Getters
  const FilePath& getWorkspacePath() const noexcept { return mWorkspacePath; }
  bool isWorkspacePathValid() const noexcept { return mWorkspacePathValid; }
  bool getWorkspaceExists() const noexcept { return mWorkspaceExists; }
  const QString& getDataDir() const noexcept { return mDataDir; }
  const std::pair<QString, QString>& getUpgradeCopyDirs() const noexcept {
    return mUpgradeCopyDirs;
  }
  bool getNeedsInitialization() const noexcept {
    return !mDataDirs.contains(mDataDir);
  }
  bool getNeedsUpgrade() const noexcept {
    return !mUpgradeCopyDirs.first.isEmpty();
  }
  bool getWorkspaceContainsNewerFileFormats() const noexcept;

  // Setters
  void setWorkspacePath(const FilePath& fp);
  void setAppLocale(const QString& locale) noexcept { mAppLocale = locale; }
  void setLengthUnit(const LengthUnit& unit) noexcept { mLengthUnit = unit; }
  void setLibraryNormOrder(const QStringList& order) noexcept {
    mLibraryNormOrder = order;
  }
  void setUserName(const QString& name) noexcept { mUserName = name; }

  // General Methods
  void initializeEmptyWorkspace() const;
  void installExampleProjects() const noexcept;

  // Operator Overloadings
  InitializeWorkspaceWizardContext& operator=(
      const InitializeWorkspaceWizardContext& rhs) = delete;

private:  // Data
  FilePath mWorkspacePath;
  bool mWorkspacePathValid;
  bool mWorkspaceExists;
  QMap<QString, Version> mDataDirs;
  QString mDataDir;
  std::pair<QString, QString> mUpgradeCopyDirs;

  // Settings.
  QString mAppLocale;
  LengthUnit mLengthUnit;
  QStringList mLibraryNormOrder;
  QString mUserName;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
