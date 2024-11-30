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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_START_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_START_H

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
class KiCadLibraryImportWizardPage_Start;
}

/*******************************************************************************
 *  Class KiCadLibraryImportWizardPage_Start
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizardPage_Start class
 */
class KiCadLibraryImportWizardPage_Start final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryImportWizardPage_Start() = delete;
  KiCadLibraryImportWizardPage_Start(
      const KiCadLibraryImportWizardPage_Start& other) = delete;
  KiCadLibraryImportWizardPage_Start(
      std::shared_ptr<KiCadLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizardPage_Start() noexcept;

  // Operator Overloadings
  KiCadLibraryImportWizardPage_Start& operator=(
      const KiCadLibraryImportWizardPage_Start& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::KiCadLibraryImportWizardPage_Start> mUi;
  std::shared_ptr<KiCadLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
