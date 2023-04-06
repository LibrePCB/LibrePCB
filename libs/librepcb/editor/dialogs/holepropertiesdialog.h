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

#ifndef LIBREPCB_EDITOR_HOLEPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_HOLEPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Hole;
class Hole;
class LengthUnit;

namespace editor {

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
  HolePropertiesDialog(BI_Hole& hole, UndoStack& undoStack,
                       const LengthUnit& lengthUnit,
                       const QString& settingsPrefix,
                       QWidget* parent = nullptr) noexcept;
  ~HolePropertiesDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  HolePropertiesDialog& operator=(const HolePropertiesDialog& rhs) = delete;

private:  // Methods
  HolePropertiesDialog(Hole* libObj, BI_Hole* boardObj, UndoStack& undoStack,
                       const LengthUnit& lengthUnit,
                       const QString& settingsPrefix,
                       QWidget* parent = nullptr) noexcept;
  template <typename T>
  void load(const T& obj) noexcept;
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;
  template <typename T>
  void applyChanges(T& cmd);

private:  // Data
  Hole* mLibraryObj;
  BI_Hole* mBoardObj;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::HolePropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
