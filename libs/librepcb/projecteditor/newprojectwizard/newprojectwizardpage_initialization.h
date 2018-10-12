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

#ifndef LIBREPCB_PROJECT_NEWPROJECTWIZARDPAGE_INITIALIZATION_H
#define LIBREPCB_PROJECT_NEWPROJECTWIZARDPAGE_INITIALIZATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/

namespace librepcb {
namespace project {
namespace editor {

namespace Ui {
class NewProjectWizardPage_Initialization;
}

/*******************************************************************************
 *  Class NewProjectWizardPage_Initialization
 ******************************************************************************/

/**
 * @brief The NewProjectWizardPage_Initialization class
 *
 * @author ubruhin
 * @date 2016-08-13
 */
class NewProjectWizardPage_Initialization final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor

  explicit NewProjectWizardPage_Initialization(
      QWidget* parent = nullptr) noexcept;
  NewProjectWizardPage_Initialization(
      const NewProjectWizardPage_Initialization& other) = delete;
  ~NewProjectWizardPage_Initialization() noexcept;

  // Getters
  bool    getCreateSchematic() const noexcept;
  QString getSchematicName() const noexcept;
  QString getSchematicFileName() const noexcept;
  bool    getCreateBoard() const noexcept;
  QString getBoardName() const noexcept;
  QString getBoardFileName() const noexcept;

  // Operator Overloadings
  NewProjectWizardPage_Initialization& operator       =(
      const NewProjectWizardPage_Initialization& rhs) = delete;

private:  // GUI Action Handlers
  void schematicNameChanged(const QString& name) noexcept;
  void boardNameChanged(const QString& name) noexcept;

private:  // Methods
  bool isComplete() const noexcept override;

private:  // Data
  QScopedPointer<Ui::NewProjectWizardPage_Initialization> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_NEWPROJECTWIZARDPAGE_INITIALIZATION_H
