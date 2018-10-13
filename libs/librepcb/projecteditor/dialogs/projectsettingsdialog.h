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

#ifndef LIBREPCB_PROJECT_PROJECTSETTINGSDIALOG_H
#define LIBREPCB_PROJECT_PROJECTSETTINGSDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace project {

class ProjectSettings;
class CmdProjectSettingsChange;

namespace editor {

namespace Ui {
class ProjectSettingsDialog;
}

/*******************************************************************************
 *  Class ProjectSettingsDialog
 ******************************************************************************/

/**
 * @brief The ProjectSettingsDialog class
 */
class ProjectSettingsDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit ProjectSettingsDialog(ProjectSettings& settings,
                                 UndoStack&       undoStack,
                                 QWidget*         parent = 0) noexcept;
  ~ProjectSettingsDialog() noexcept;

private slots:

  // GUI Events
  void on_buttonBox_clicked(QAbstractButton* button);
  void on_btnLocaleAdd_clicked();
  void on_btnLocaleRemove_clicked();
  void on_btnLocaleUp_clicked();
  void on_btnLocaleDown_clicked();
  void on_btnNormAdd_clicked();
  void on_btnNormRemove_clicked();
  void on_btnNormUp_clicked();
  void on_btnNormDown_clicked();

private:
  // make some methods inaccessible...
  ProjectSettingsDialog();
  ProjectSettingsDialog(const ProjectSettingsDialog& other);
  ProjectSettingsDialog& operator=(const ProjectSettingsDialog& rhs);

  // Private Methods
  void accept();
  void reject();
  bool applySettings() noexcept;
  bool restoreDefaultSettings() noexcept;
  void updateGuiFromSettings() noexcept;

  // General
  ProjectSettings&           mSettings;
  Ui::ProjectSettingsDialog* mUi;
  UndoStack&                 mUndoStack;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_PROJECTSETTINGSDIALOG_H
