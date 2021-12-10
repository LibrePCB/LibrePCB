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

#ifndef LIBREPCB_INITIALIZEWORKSPACEWIZARD_INITIALIZEWORKSPACEWIZARD_CHOOSEIMPORTVERSION_H
#define LIBREPCB_INITIALIZEWORKSPACEWIZARD_INITIALIZEWORKSPACEWIZARD_CHOOSEIMPORTVERSION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "initializeworkspacewizardcontext.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace application {

namespace Ui {
class InitializeWorkspaceWizard_ChooseImportVersion;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard_ChooseImportVersion
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard_ChooseImportVersion class
 */
class InitializeWorkspaceWizard_ChooseImportVersion final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard_ChooseImportVersion(
      InitializeWorkspaceWizardContext& context, QWidget* parent = 0) noexcept;
  InitializeWorkspaceWizard_ChooseImportVersion(
      const InitializeWorkspaceWizard_ChooseImportVersion& other) = delete;
  ~InitializeWorkspaceWizard_ChooseImportVersion() noexcept;

  // Inherited from QWizardPage
  bool validatePage() noexcept override;
  int nextId() const noexcept override;

  // Operator Overloadings
  InitializeWorkspaceWizard_ChooseImportVersion& operator=(
      const InitializeWorkspaceWizard_ChooseImportVersion& rhs) = delete;

private:
  void cbxVersionCurrentIndexChanged(int index) noexcept;

private:
  InitializeWorkspaceWizardContext& mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard_ChooseImportVersion> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif
