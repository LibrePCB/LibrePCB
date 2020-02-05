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

#ifndef LIBREPCB_APPLICATION_INITIALIZEWORKSPACEWIZARD_CHOOSESETTINGS_H
#define LIBREPCB_APPLICATION_INITIALIZEWORKSPACEWIZARD_CHOOSESETTINGS_H

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
class InitializeWorkspaceWizard_ChooseSettings;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard_ChooseSettings
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard_ChooseSettings class
 */
class InitializeWorkspaceWizard_ChooseSettings final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard_ChooseSettings(
      InitializeWorkspaceWizardContext& context, QWidget* parent = 0) noexcept;
  InitializeWorkspaceWizard_ChooseSettings(
      const InitializeWorkspaceWizard_ChooseSettings& other) = delete;
  ~InitializeWorkspaceWizard_ChooseSettings() noexcept;

  // Inherited from QWizardPage
  bool validatePage() noexcept override;
  int  nextId() const noexcept override;

  // Operator Overloadings
  InitializeWorkspaceWizard_ChooseSettings& operator       =(
      const InitializeWorkspaceWizard_ChooseSettings& rhs) = delete;

private:
  void cbxVersionCurrentIndexChanged(int index) noexcept;

private:
  InitializeWorkspaceWizardContext&                            mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard_ChooseSettings> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif  // LIBREPCB_APPLICATION_INITIALIZEWORKSPACEWIZARD_CHOOSESETTINGS_H
