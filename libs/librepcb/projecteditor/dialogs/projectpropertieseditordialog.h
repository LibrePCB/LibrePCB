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

#ifndef LIBREPCB_PROJECT_PROJECTPROPERTIESEDITORDIALOG_H
#define LIBREPCB_PROJECT_PROJECTPROPERTIESEDITORDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace project {

class ProjectMetadata;

namespace editor {

namespace Ui {
class ProjectPropertiesEditorDialog;
}

/*******************************************************************************
 *  Class ProjectPropertiesEditorDialog
 ******************************************************************************/

/**
 * @brief The ProjectPropertiesEditorDialog class
 */
class ProjectPropertiesEditorDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectPropertiesEditorDialog() = delete;
  ProjectPropertiesEditorDialog(const ProjectPropertiesEditorDialog& other) =
      delete;
  ProjectPropertiesEditorDialog(ProjectMetadata& metadata, UndoStack& undoStack,
                                QWidget* parent) noexcept;
  ~ProjectPropertiesEditorDialog() noexcept;

  // Operator Overloadings
  ProjectPropertiesEditorDialog& operator       =(
      const ProjectPropertiesEditorDialog& rhs) = delete;

private:  // Methods
  void keyPressEvent(QKeyEvent* e);
  void accept();
  bool applyChanges() noexcept;

private:  // Data
  ProjectMetadata&                                  mMetadata;
  UndoStack&                                        mUndoStack;
  QScopedPointer<Ui::ProjectPropertiesEditorDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_PROJECTPROPERTIESEDITORDIALOG_H
