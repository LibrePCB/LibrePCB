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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_PARSE_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_PARSE_H

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
class KiCadLibraryImportWizardPage_Parse;
}

/*******************************************************************************
 *  Class KiCadLibraryImportWizardPage_Parse
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizardPage_Parse class
 */
class KiCadLibraryImportWizardPage_Parse final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryImportWizardPage_Parse() = delete;
  KiCadLibraryImportWizardPage_Parse(
      const KiCadLibraryImportWizardPage_Parse& other) = delete;
  KiCadLibraryImportWizardPage_Parse(
      std::shared_ptr<KiCadLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizardPage_Parse() noexcept;

  // General Methods
  virtual void initializePage() override;
  virtual bool isComplete() const override;

  // Operator Overloadings
  KiCadLibraryImportWizardPage_Parse& operator=(
      const KiCadLibraryImportWizardPage_Parse& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::KiCadLibraryImportWizardPage_Parse> mUi;
  std::shared_ptr<KiCadLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
