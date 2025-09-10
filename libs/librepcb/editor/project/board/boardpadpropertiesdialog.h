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

#ifndef LIBREPCB_EDITOR_BOARDPADPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_BOARDPADPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/padhole.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Pad;
class LengthUnit;

namespace editor {

class UndoStack;

namespace Ui {
class BoardPadPropertiesDialog;
}

/*******************************************************************************
 *  Class BoardPadPropertiesDialog
 ******************************************************************************/

/**
 * @brief The BoardPadPropertiesDialog class
 */
class BoardPadPropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardPadPropertiesDialog() = delete;
  BoardPadPropertiesDialog(const BoardPadPropertiesDialog& other) = delete;
  BoardPadPropertiesDialog(BI_Pad& pad, UndoStack& undoStack,
                           const LengthUnit& lengthUnit,
                           const QString& settingsPrefix,
                           QWidget* parent = nullptr) noexcept;
  ~BoardPadPropertiesDialog() noexcept;

  // Operator Overloadings
  BoardPadPropertiesDialog& operator=(const BoardPadPropertiesDialog& rhs) =
      delete;

private:  // Methods
  void updateShapeDependentWidgets(bool checked) noexcept;
  void updateAbsoluteRadius() noexcept;
  void updateRelativeRadius() noexcept;
  void applyRecommendedRadius() noexcept;
  void addHole() noexcept;
  void removeSelectedHole() noexcept;
  void removeAllHoles() noexcept;
  void updateGeneralTabHoleWidgets() noexcept;
  void setSelectedHole(int index) noexcept;
  void applyTypicalThtProperties() noexcept;
  void applyTypicalSmtProperties() noexcept;
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;

private:  // Data
  BI_Pad& mPad;
  UndoStack& mUndoStack;
  PadHoleList mHoles;
  int mSelectedHoleIndex;
  QScopedPointer<Ui::BoardPadPropertiesDialog> mUi;
  Path mAutoCustomOutline;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
