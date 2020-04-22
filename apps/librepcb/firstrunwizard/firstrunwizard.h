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

#ifndef LIBREPCB_FIRSTRUNWIZARD_H
#define LIBREPCB_FIRSTRUNWIZARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace application {

namespace Ui {
class FirstRunWizard;
}

/*******************************************************************************
 *  Class FirstRunWizard
 ******************************************************************************/

/**
 * @brief The FirstRunWizard class
 */
class FirstRunWizard final : public QWizard {
  Q_OBJECT

  enum PageId {
    Page_Welcome,
    Page_WorkspacePath,
    Page_WorkspaceSettings,
  };

public:
  // Constructors / Destructor
  explicit FirstRunWizard(QWidget* parent = 0) noexcept;
  ~FirstRunWizard() noexcept;

  // Getters
  bool     getCreateNewWorkspace() const noexcept;
  FilePath getWorkspaceFilePath() const noexcept;
  QString  getNewWorkspaceUserName() const noexcept;

  // General Methods
  void skipWelcomePage() noexcept;

  // Inherited from QWizard
  int nextId() const override;

private:
  // Private Methods
  Q_DISABLE_COPY(FirstRunWizard)

  // Private Membervariables
  QScopedPointer<Ui::FirstRunWizard> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif  // LIBREPCB_FIRSTRUNWIZARD_H
