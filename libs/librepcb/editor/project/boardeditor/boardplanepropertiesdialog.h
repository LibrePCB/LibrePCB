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

#ifndef LIBREPCB_EDITOR_BOARDPLANEPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_BOARDPLANEPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Plane;
class LengthUnit;
class Project;

namespace editor {

class UndoCommand;
class UndoStack;

namespace Ui {
class BoardPlanePropertiesDialog;
}

/*******************************************************************************
 *  Class BoardPlanePropertiesDialog
 ******************************************************************************/

/**
 * @brief The BoardPlanePropertiesDialog class
 */
class BoardPlanePropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardPlanePropertiesDialog() = delete;
  BoardPlanePropertiesDialog(const BoardPlanePropertiesDialog& other) = delete;
  BoardPlanePropertiesDialog(Project& project, BI_Plane& plane,
                             UndoStack& undoStack, const LengthUnit& lengthUnit,
                             const QString& settingsPrefix,
                             QWidget* parent) noexcept;
  ~BoardPlanePropertiesDialog() noexcept;

private:  // GUI Events
  void buttonBoxClicked(QAbstractButton* button) noexcept;

private:  // Methods
  bool applyChanges() noexcept;

  // General
  Project& mProject;
  BI_Plane& mPlane;
  QScopedPointer<Ui::BoardPlanePropertiesDialog> mUi;
  UndoStack& mUndoStack;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
