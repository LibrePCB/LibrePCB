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

#ifndef LIBREPCB_EDITOR_NEWPROJECTWIZARD_H
#define LIBREPCB_EDITOR_NEWPROJECTWIZARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;
class Workspace;

namespace editor {

class NewProjectWizardPage_Initialization;
class NewProjectWizardPage_Metadata;

//

class NewProjectWizardPage_VersionControl;

namespace Ui {
class NewProjectWizard;
}

/*******************************************************************************
 *  Class NewProjectWizard
 ******************************************************************************/

/**
 * @brief The NewProjectWizard class
 */
class NewProjectWizard final : public QWizard {
  Q_OBJECT

public:
  // Constructors / Destructor
  NewProjectWizard() = delete;
  NewProjectWizard(const NewProjectWizard& other) = delete;
  explicit NewProjectWizard(const Workspace& ws,
                            QWidget* parent = nullptr) noexcept;
  ~NewProjectWizard() noexcept;

  // Setters
  void setLocation(const FilePath& dir) noexcept;

  // General Methods
  std::unique_ptr<Project> createProject() const;

  // Operator Overloadings
  NewProjectWizard& operator=(const NewProjectWizard& rhs) = delete;

private:  // Data
  const Workspace& mWorkspace;
  QScopedPointer<Ui::NewProjectWizard> mUi;
  NewProjectWizardPage_Metadata* mPageMetadata;
  NewProjectWizardPage_Initialization* mPageInitialization;
  // NewProjectWizardPage_VersionControl* mPageVersionControl;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
