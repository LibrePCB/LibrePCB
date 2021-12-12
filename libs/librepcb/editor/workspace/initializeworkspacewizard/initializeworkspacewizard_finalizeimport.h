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

#ifndef LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_FINALIZEIMPORT_H
#define LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_FINALIZEIMPORT_H

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
class InitializeWorkspaceWizard_FinalizeImport;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard_FinalizeImport
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard_FinalizeImport class
 */
class InitializeWorkspaceWizard_FinalizeImport final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard_FinalizeImport(
      InitializeWorkspaceWizardContext& context, QWidget* parent = 0) noexcept;
  InitializeWorkspaceWizard_FinalizeImport(
      const InitializeWorkspaceWizard_FinalizeImport& other) = delete;
  ~InitializeWorkspaceWizard_FinalizeImport() noexcept;

  // Inherited from QWizardPage
  void initializePage() noexcept override;
  bool isComplete() const noexcept override;
  int nextId() const noexcept override;

  // Operator Overloadings
  InitializeWorkspaceWizard_FinalizeImport& operator=(
      const InitializeWorkspaceWizard_FinalizeImport& rhs) = delete;

private:
  void startImport() noexcept;
  void importFailed(const QString& error) noexcept;
  void importSucceeded() noexcept;

private:
  InitializeWorkspaceWizardContext& mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard_FinalizeImport> mUi;
  std::unique_ptr<AsyncCopyOperation> mCopyOperation;
  bool mImportSucceeded;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
