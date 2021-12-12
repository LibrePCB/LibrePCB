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

#ifndef LIBREPCB_EDITOR_NEWPROJECTWIZARDPAGE_VERSIONCONTROL_H
#define LIBREPCB_EDITOR_NEWPROJECTWIZARDPAGE_VERSIONCONTROL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class NewProjectWizardPage_VersionControl;
}

/*******************************************************************************
 *  Class NewProjectWizardPage_VersionControl
 ******************************************************************************/

/**
 * @brief The NewProjectWizardPage_VersionControl class
 */
class NewProjectWizardPage_VersionControl final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor

  explicit NewProjectWizardPage_VersionControl(
      QWidget* parent = nullptr) noexcept;
  NewProjectWizardPage_VersionControl(
      const NewProjectWizardPage_VersionControl& other) = delete;
  ~NewProjectWizardPage_VersionControl() noexcept;

  // Getters
  bool getInitGitRepository() const noexcept;

  // Operator Overloadings
  NewProjectWizardPage_VersionControl& operator=(
      const NewProjectWizardPage_VersionControl& rhs) = delete;

private:  // GUI Action Handlers
private:  // Methods
private:  // Data
  QScopedPointer<Ui::NewProjectWizardPage_VersionControl> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
