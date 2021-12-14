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

#ifndef LIBREPCB_EDITOR_EAGLELIBRARYIMPORTWIZARD_H
#define LIBREPCB_EDITOR_EAGLELIBRARYIMPORTWIZARD_H

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

class EagleLibraryImportWizardContext;

namespace Ui {
class EagleLibraryImportWizard;
}

/*******************************************************************************
 *  Class EagleLibraryImportWizard
 ******************************************************************************/

/**
 * @brief The EagleLibraryImportWizard class
 */
class EagleLibraryImportWizard final : public QWizard {
  Q_OBJECT

public:
  // Constructors / Destructor
  EagleLibraryImportWizard(const EagleLibraryImportWizard& other) = delete;
  EagleLibraryImportWizard(Workspace& workspace, const FilePath& dstLibFp,
                           QWidget* parent = nullptr) noexcept;
  ~EagleLibraryImportWizard() noexcept;

  // General Methods
  void reject() noexcept override;

  // Operator Overloadings
  EagleLibraryImportWizard& operator=(const EagleLibraryImportWizard& rhs) =
      delete;

private:  // Data
  QScopedPointer<Ui::EagleLibraryImportWizard> mUi;
  std::shared_ptr<EagleLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
