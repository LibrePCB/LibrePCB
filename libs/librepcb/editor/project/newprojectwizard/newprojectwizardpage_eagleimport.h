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

#ifndef LIBREPCB_EDITOR_NEWPROJECTWIZARDPAGE_EAGLEIMPORT_H
#define LIBREPCB_EDITOR_NEWPROJECTWIZARDPAGE_EAGLEIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace eagleimport {
class EagleProjectImport;
}

class Project;
class FilePath;
class Workspace;

namespace editor {

namespace Ui {
class NewProjectWizardPage_EagleImport;
}

class WaitingSpinnerWidget;

/*******************************************************************************
 *  Class NewProjectWizardPage_EagleImport
 ******************************************************************************/

/**
 * @brief The NewProjectWizardPage_EagleImport class
 */
class NewProjectWizardPage_EagleImport final : public QWizardPage {
  Q_OBJECT

  struct ParserResult {
    std::shared_ptr<eagleimport::EagleProjectImport> import;
    QStringList messages;
  };

public:
  // Constructors / Destructor
  explicit NewProjectWizardPage_EagleImport(const Workspace& ws,
                                            QWidget* parent = nullptr) noexcept;
  NewProjectWizardPage_EagleImport(
      const NewProjectWizardPage_EagleImport& other) = delete;
  ~NewProjectWizardPage_EagleImport() noexcept;

  // General Methods
  void import(Project& project);

  // Operator Overloadings
  NewProjectWizardPage_EagleImport& operator=(
      const NewProjectWizardPage_EagleImport& rhs) = delete;

signals:
  void projectSelected(const QString& name) const;

private:  // Methods
  void updateStatus() noexcept;
  static NewProjectWizardPage_EagleImport::ParserResult parseAsync(
      std::shared_ptr<eagleimport::EagleProjectImport> import,
      const FilePath& schFp, const FilePath& brdFp) noexcept;
  bool isComplete() const noexcept override;

private:  // Data
  const Workspace& mWorkspace;
  QScopedPointer<Ui::NewProjectWizardPage_EagleImport> mUi;
  QScopedPointer<WaitingSpinnerWidget> mWaitingSpinner;
  QString mCurrentSchematic;
  QString mCurrentBoard;
  QFuture<ParserResult> mFuture;
  std::shared_ptr<eagleimport::EagleProjectImport> mImport;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
