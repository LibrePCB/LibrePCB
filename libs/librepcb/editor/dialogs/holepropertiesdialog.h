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

#ifndef LIBREPCB_COMMON_HOLEPROPERTIESDIALOG_H
#define LIBREPCB_COMMON_HOLEPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Hole;
class LengthUnit;
class UndoStack;

namespace Ui {
class HolePropertiesDialog;
}

/*******************************************************************************
 *  Class HolePropertiesDialog
 ******************************************************************************/

/**
 * @brief The HolePropertiesDialog class
 */
class HolePropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  HolePropertiesDialog() = delete;
  HolePropertiesDialog(const HolePropertiesDialog& other) = delete;
  HolePropertiesDialog(Hole& hole, UndoStack& undoStack,
                       const LengthUnit& lengthUnit,
                       const QString& settingsPrefix,
                       QWidget* parent = nullptr) noexcept;
  ~HolePropertiesDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  HolePropertiesDialog& operator=(const HolePropertiesDialog& rhs) = delete;

private:  // Methods
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;

private:  // Data
  Hole& mHole;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::HolePropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
