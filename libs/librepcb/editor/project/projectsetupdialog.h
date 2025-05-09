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

#ifndef LIBREPCB_EDITOR_PROJECTSETUPDIALOG_H
#define LIBREPCB_EDITOR_PROJECTSETUPDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attribute.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

namespace editor {

class UndoStack;

namespace Ui {
class ProjectSetupDialog;
}

/*******************************************************************************
 *  Class ProjectSetupDialog
 ******************************************************************************/

/**
 * @brief The ProjectSetupDialog class
 */
class ProjectSetupDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectSetupDialog() = delete;
  ProjectSetupDialog(const ProjectSetupDialog& other) = delete;
  ProjectSetupDialog(Project& project, UndoStack& undoStack,
                     QWidget* parent = nullptr) noexcept;
  ~ProjectSetupDialog();

  // Operator Overloadings
  ProjectSetupDialog& operator=(const ProjectSetupDialog& rhs) = delete;

private:  // Methods
  void buttonBoxClicked(QAbstractButton* button);
  void load() noexcept;
  bool apply() noexcept;

private:  // Date
  Project& mProject;
  UndoStack& mUndoStack;
  AttributeList mAttributes;
  QScopedPointer<Ui::ProjectSetupDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
