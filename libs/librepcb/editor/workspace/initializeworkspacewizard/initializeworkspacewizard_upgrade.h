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

#ifndef LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_UPGRADE_H
#define LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_UPGRADE_H

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

class AsyncCopyOperation;

namespace editor {

namespace Ui {
class InitializeWorkspaceWizard_Upgrade;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard_Upgrade
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard_Upgrade class
 */
class InitializeWorkspaceWizard_Upgrade final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard_Upgrade(
      InitializeWorkspaceWizardContext& context, QWidget* parent = 0) noexcept;
  InitializeWorkspaceWizard_Upgrade(
      const InitializeWorkspaceWizard_Upgrade& other) = delete;
  ~InitializeWorkspaceWizard_Upgrade() noexcept;

  // Inherited from QWizardPage
  void initializePage() noexcept override;
  bool validatePage() noexcept override;
  int nextId() const noexcept override;

  // Operator Overloadings
  InitializeWorkspaceWizard_Upgrade& operator=(
      const InitializeWorkspaceWizard_Upgrade& rhs) = delete;

private:
  InitializeWorkspaceWizardContext& mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard_Upgrade> mUi;
  QScopedPointer<AsyncCopyOperation> mCopyOperation;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
