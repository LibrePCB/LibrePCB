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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_CHOOSELIBRARY_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_CHOOSELIBRARY_H

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

class MessageLogger;

namespace editor {

class KiCadLibraryImportWizardContext;

namespace Ui {
class KiCadLibraryImportWizardPage_ChooseLibrary;
}

/*******************************************************************************
 *  Class KiCadLibraryImportWizardPage_ChooseLibrary
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizardPage_ChooseLibrary class
 */
class KiCadLibraryImportWizardPage_ChooseLibrary final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryImportWizardPage_ChooseLibrary() = delete;
  KiCadLibraryImportWizardPage_ChooseLibrary(
      const KiCadLibraryImportWizardPage_ChooseLibrary& other) = delete;
  KiCadLibraryImportWizardPage_ChooseLibrary(
      std::shared_ptr<KiCadLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizardPage_ChooseLibrary() noexcept;

  // General Methods
  virtual void initializePage() override;
  virtual bool isComplete() const override;

  // Operator Overloadings
  KiCadLibraryImportWizardPage_ChooseLibrary& operator=(
      const KiCadLibraryImportWizardPage_ChooseLibrary& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::KiCadLibraryImportWizardPage_ChooseLibrary> mUi;
  std::shared_ptr<KiCadLibraryImportWizardContext> mContext;
  std::shared_ptr<MessageLogger> mLogger;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
