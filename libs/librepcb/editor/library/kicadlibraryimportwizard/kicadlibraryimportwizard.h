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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARD_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARD_H

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

class FilePath;
class Workspace;

namespace editor {

class KiCadLibraryImportWizardContext;

namespace Ui {
class KiCadLibraryImportWizard;
}

/*******************************************************************************
 *  Class KiCadLibraryImportWizard
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizard class
 */
class KiCadLibraryImportWizard final : public QWizard {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryImportWizard(const KiCadLibraryImportWizard& other) = delete;
  KiCadLibraryImportWizard(Workspace& workspace, const FilePath& dstLibFp,
                           QWidget* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizard() noexcept;

  // General Methods
  void reject() noexcept override;

  // Operator Overloadings
  KiCadLibraryImportWizard& operator=(const KiCadLibraryImportWizard& rhs) =
      delete;

private:  // Data
  QScopedPointer<Ui::KiCadLibraryImportWizard> mUi;
  std::shared_ptr<KiCadLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
