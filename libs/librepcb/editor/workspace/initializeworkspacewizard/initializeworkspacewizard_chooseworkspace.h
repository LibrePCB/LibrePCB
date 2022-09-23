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

#ifndef LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_CHOOSEWORKSPACE_H
#define LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_CHOOSEWORKSPACE_H

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
namespace editor {

namespace Ui {
class InitializeWorkspaceWizard_ChooseWorkspace;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard_ChooseWorkspace
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard_ChooseWorkspace class
 */
class InitializeWorkspaceWizard_ChooseWorkspace final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard_ChooseWorkspace(
      InitializeWorkspaceWizardContext& context, QWidget* parent = 0) noexcept;
  InitializeWorkspaceWizard_ChooseWorkspace(
      const InitializeWorkspaceWizard_ChooseWorkspace& other) = delete;
  ~InitializeWorkspaceWizard_ChooseWorkspace() noexcept;

  // Inherited Methods
  void initializePage() noexcept override;
  bool isComplete() const noexcept override;
  int nextId() const noexcept override;

  // Operator Overloadings
  InitializeWorkspaceWizard_ChooseWorkspace& operator=(
      const InitializeWorkspaceWizard_ChooseWorkspace& rhs) = delete;

private:  // Methods
  void updateWorkspacePath() noexcept;

private:  // Data
  InitializeWorkspaceWizardContext& mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard_ChooseWorkspace> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
