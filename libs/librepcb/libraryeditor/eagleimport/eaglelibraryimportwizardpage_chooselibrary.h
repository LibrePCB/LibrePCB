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

#ifndef LIBREPCB_LIBRARYEDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_CHOOSELIBRARY_H
#define LIBREPCB_LIBRARYEDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_CHOOSELIBRARY_H

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
class EagleLibraryImportWizardPage_ChooseLibrary;
}

/*******************************************************************************
 *  Class EagleLibraryImportWizardPage_ChooseLibrary
 ******************************************************************************/

/**
 * @brief The EagleLibraryImportWizardPage_ChooseLibrary class
 */
class EagleLibraryImportWizardPage_ChooseLibrary final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  EagleLibraryImportWizardPage_ChooseLibrary() = delete;
  EagleLibraryImportWizardPage_ChooseLibrary(
      const EagleLibraryImportWizardPage_ChooseLibrary& other) = delete;
  EagleLibraryImportWizardPage_ChooseLibrary(
      std::shared_ptr<EagleLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~EagleLibraryImportWizardPage_ChooseLibrary() noexcept;

  // General Methods
  virtual void initializePage() override;
  virtual bool isComplete() const override;

  // Operator Overloadings
  EagleLibraryImportWizardPage_ChooseLibrary& operator=(
      const EagleLibraryImportWizardPage_ChooseLibrary& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::EagleLibraryImportWizardPage_ChooseLibrary> mUi;
  std::shared_ptr<EagleLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
