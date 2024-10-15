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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_SETOPTIONS_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_SETOPTIONS_H

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
namespace editor {

class KiCadLibraryImportWizardContext;

namespace Ui {
class KiCadLibraryImportWizardPage_SetOptions;
}

/*******************************************************************************
 *  Class KiCadLibraryImportWizardPage_SetOptions
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizardPage_SetOptions class
 */
class KiCadLibraryImportWizardPage_SetOptions final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryImportWizardPage_SetOptions() = delete;
  KiCadLibraryImportWizardPage_SetOptions(
      const KiCadLibraryImportWizardPage_SetOptions& other) = delete;
  KiCadLibraryImportWizardPage_SetOptions(
      std::shared_ptr<KiCadLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizardPage_SetOptions() noexcept;

  // General Methods
  virtual void initializePage() override;

  // Operator Overloadings
  KiCadLibraryImportWizardPage_SetOptions& operator=(
      const KiCadLibraryImportWizardPage_SetOptions& rhs) = delete;

private:  // Methods
  void updateComponentCategoryTreeLabel() noexcept;
  void updatePackageCategoryTreeLabel() noexcept;

private:  // Data
  QScopedPointer<Ui::KiCadLibraryImportWizardPage_SetOptions> mUi;
  std::shared_ptr<KiCadLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
