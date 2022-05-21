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

#ifndef LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_WELCOME_H
#define LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_WELCOME_H

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
class InitializeWorkspaceWizard_Welcome;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard_Welcome
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard_Welcome class
 */
class InitializeWorkspaceWizard_Welcome final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard_Welcome(
      InitializeWorkspaceWizardContext& context, QWidget* parent = 0) noexcept;
  InitializeWorkspaceWizard_Welcome(
      const InitializeWorkspaceWizard_Welcome& other) = delete;
  ~InitializeWorkspaceWizard_Welcome() noexcept;

  // Operator Overloadings
  InitializeWorkspaceWizard_Welcome& operator=(
      const InitializeWorkspaceWizard_Welcome& rhs) = delete;

private:
  InitializeWorkspaceWizardContext& mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard_Welcome> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
