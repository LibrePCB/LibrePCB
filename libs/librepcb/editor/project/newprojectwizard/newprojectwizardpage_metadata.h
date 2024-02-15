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

#ifndef LIBREPCB_EDITOR_NEWPROJECTWIZARDPAGE_METADATA_H
#define LIBREPCB_EDITOR_NEWPROJECTWIZARDPAGE_METADATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

namespace Ui {
class NewProjectWizardPage_Metadata;
}

/*******************************************************************************
 *  Class NewProjectWizardPage_Metadata
 ******************************************************************************/

/**
 * @brief The NewProjectWizardPage_Metadata class
 */
class NewProjectWizardPage_Metadata final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit NewProjectWizardPage_Metadata(const Workspace& ws,
                                         QWidget* parent = nullptr) noexcept;
  NewProjectWizardPage_Metadata(const NewProjectWizardPage_Metadata& other) =
      delete;
  ~NewProjectWizardPage_Metadata() noexcept;

  // Setters
  void setProjectName(const QString& name) noexcept;
  void setLocationOverride(const FilePath& dir) noexcept;

  // Getters
  QString getProjectName() const noexcept;
  QString getProjectAuthor() const noexcept;
  bool isLicenseSet() const noexcept;
  FilePath getProjectLicenseFilePath() const noexcept;
  FilePath getFullFilePath() const noexcept { return mFullFilePath; }

  // Operator Overloadings
  NewProjectWizardPage_Metadata& operator=(
      const NewProjectWizardPage_Metadata& rhs) = delete;

private:  // GUI Action Handlers
  void nameChanged(const QString& name) noexcept;
  void pathChanged(const QString& fp) noexcept;
  void chooseLocationClicked() noexcept;

private:  // Methods
  bool isComplete() const noexcept override;
  bool validatePage() noexcept override;
  void setStatusMessage(const QString& msg) const noexcept;

private:  // Data
  const Workspace& mWorkspace;
  QScopedPointer<Ui::NewProjectWizardPage_Metadata> mUi;
  FilePath mLocation;
  bool mLocationOverridden;
  FilePath mFullFilePath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
