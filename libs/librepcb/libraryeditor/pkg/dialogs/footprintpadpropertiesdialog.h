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

#ifndef LIBREPCB_LIBRARY_EDITOR_FOOTPRINTPADPROPERTIESDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_FOOTPRINTPADPROPERTIESDIALOG_H

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
class LengthUnit;

namespace library {

class Package;
class Footprint;
class FootprintPad;

namespace editor {

namespace Ui {
class FootprintPadPropertiesDialog;
}

/*******************************************************************************
 *  Class FootprintPadPropertiesDialog
 ******************************************************************************/

/**
 * @brief The FootprintPadPropertiesDialog class
 */
class FootprintPadPropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  FootprintPadPropertiesDialog() = delete;
  FootprintPadPropertiesDialog(const FootprintPadPropertiesDialog& other) =
      delete;
  FootprintPadPropertiesDialog(const Package& pkg, FootprintPad& pad,
                               UndoStack& undoStack,
                               const LengthUnit& lengthUnit,
                               const QString& settingsPrefix,
                               QWidget* parent = nullptr) noexcept;
  ~FootprintPadPropertiesDialog() noexcept;

  // Operator Overloadings
  FootprintPadPropertiesDialog& operator=(
      const FootprintPadPropertiesDialog& rhs) = delete;

private:  // Methods
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;

private:  // Data
  FootprintPad& mPad;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::FootprintPadPropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_FOOTPRINTPADPROPERTIESDIALOG_H
