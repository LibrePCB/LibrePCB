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

#ifndef LIBREPCB_EDITOR_DEVICEINSTANCEPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_DEVICEINSTANCEPROPERTIESDIALOG_H

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

class BI_Device;
class LengthUnit;
class Part;
class Project;
class Workspace;

namespace editor {

class UndoCommand;
class UndoStack;

namespace Ui {
class DeviceInstancePropertiesDialog;
}

/*******************************************************************************
 *  Class DeviceInstancePropertiesDialog
 ******************************************************************************/

/**
 * @brief The DeviceInstancePropertiesDialog class
 */
class DeviceInstancePropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  DeviceInstancePropertiesDialog() = delete;
  DeviceInstancePropertiesDialog(const DeviceInstancePropertiesDialog& other) =
      delete;
  DeviceInstancePropertiesDialog(const Workspace& ws, Project& project,
                                 BI_Device& device, UndoStack& undoStack,
                                 const LengthUnit& lengthUnit,
                                 const QString& settingsPrefix,
                                 QWidget* parent) noexcept;
  ~DeviceInstancePropertiesDialog() noexcept;

  // Operator Overloadings
  DeviceInstancePropertiesDialog& operator=(
      const DeviceInstancePropertiesDialog& rhs) = delete;

private:  // Methods
  void setSelectedPart(std::shared_ptr<Part> part) noexcept;
  void buttonBoxClicked(QAbstractButton* button) noexcept;
  void keyPressEvent(QKeyEvent* e) noexcept override;
  void accept() noexcept override;
  bool applyChanges() noexcept;

private:  // Data
  const Workspace& mWorkspace;
  Project& mProject;
  BI_Device& mDevice;
  UndoStack& mUndoStack;
  AttributeList mAttributes;
  std::shared_ptr<Part> mSelectedPart;  // Avoid dangling reference.
  QScopedPointer<Ui::DeviceInstancePropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
