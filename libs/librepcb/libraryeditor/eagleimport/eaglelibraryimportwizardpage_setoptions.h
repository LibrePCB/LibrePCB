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

#ifndef LIBREPCB_LIBRARYEDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_SETOPTIONS_H
#define LIBREPCB_LIBRARYEDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_SETOPTIONS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

class EagleLibraryImportWizardContext;

namespace Ui {
class EagleLibraryImportWizardPage_SetOptions;
}

/*******************************************************************************
 *  Class EagleLibraryImportWizardPage_SetOptions
 ******************************************************************************/

/**
 * @brief The EagleLibraryImportWizardPage_SetOptions class
 */
class EagleLibraryImportWizardPage_SetOptions final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  EagleLibraryImportWizardPage_SetOptions() = delete;
  EagleLibraryImportWizardPage_SetOptions(
      const EagleLibraryImportWizardPage_SetOptions& other) = delete;
  EagleLibraryImportWizardPage_SetOptions(
      std::shared_ptr<EagleLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~EagleLibraryImportWizardPage_SetOptions() noexcept;

  // General Methods
  virtual void initializePage() override;

  // Operator Overloadings
  EagleLibraryImportWizardPage_SetOptions& operator=(
      const EagleLibraryImportWizardPage_SetOptions& rhs) = delete;

private:  // Methods
  void updateComponentCategoryTreeLabel() noexcept;
  void updatePackageCategoryTreeLabel() noexcept;

private:  // Data
  QScopedPointer<Ui::EagleLibraryImportWizardPage_SetOptions> mUi;
  std::shared_ptr<EagleLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
