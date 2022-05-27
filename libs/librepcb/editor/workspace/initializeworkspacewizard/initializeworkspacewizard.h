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

#ifndef LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_H
#define LIBREPCB_EDITOR_INITIALIZEWORKSPACEWIZARD_H

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
class InitializeWorkspaceWizard;
}

/*******************************************************************************
 *  Class InitializeWorkspaceWizard
 ******************************************************************************/

/**
 * @brief The InitializeWorkspaceWizard class
 */
class InitializeWorkspaceWizard final : public QWizard {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit InitializeWorkspaceWizard(bool forceChoosePath,
                                     QWidget* parent = nullptr) noexcept;
  InitializeWorkspaceWizard(const InitializeWorkspaceWizard& other) = delete;
  ~InitializeWorkspaceWizard() noexcept;

  // Getters
  bool getNeedsToBeShown() const noexcept { return mNeedsToBeShown; }
  const FilePath& getWorkspacePath() const noexcept {
    return mContext.getWorkspacePath();
  }
  const QString& getDataDir() const noexcept { return mContext.getDataDir(); }
  bool getWorkspaceContainsNewerFileFormats() const noexcept {
    return mContext.getWorkspaceContainsNewerFileFormats();
  }

  // Setters
  void setWorkspacePath(const FilePath& fp);

  // Operator Overloadings
  InitializeWorkspaceWizard& operator=(const InitializeWorkspaceWizard& rhs) =
      delete;

private:  // Methods
  void updateStartPage() noexcept;

private:  // Data
  InitializeWorkspaceWizardContext mContext;
  QScopedPointer<Ui::InitializeWorkspaceWizard> mUi;
  bool mForceChoosePath;
  bool mNeedsToBeShown;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
