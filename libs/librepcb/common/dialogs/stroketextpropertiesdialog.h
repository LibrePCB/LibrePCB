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

#ifndef LIBREPCB_STROKETEXTPROPERTIESDIALOG_H
#define LIBREPCB_STROKETEXTPROPERTIESDIALOG_H

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
class StrokeText;
class GraphicsLayer;
class LengthUnit;

namespace Ui {
class StrokeTextPropertiesDialog;
}

/*******************************************************************************
 *  Class StrokeTextPropertiesDialog
 ******************************************************************************/

/**
 * @brief The StrokeTextPropertiesDialog class
 */
class StrokeTextPropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  StrokeTextPropertiesDialog() = delete;
  StrokeTextPropertiesDialog(const StrokeTextPropertiesDialog& other) = delete;
  StrokeTextPropertiesDialog(StrokeText& text, UndoStack& undoStack,
                             QList<GraphicsLayer*> layers,
                             const LengthUnit& lengthUnit,
                             const QString& settingsPrefix,
                             QWidget* parent = nullptr) noexcept;
  ~StrokeTextPropertiesDialog() noexcept;

  // Operator Overloadings
  StrokeTextPropertiesDialog& operator=(const StrokeTextPropertiesDialog& rhs) =
      delete;

private:  // Methods
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;
  void addLayersToCombobox(const QList<QString>& names) noexcept;
  void selectLayerNameInCombobox(const QString& name) noexcept;

private:  // Data
  StrokeText& mText;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::StrokeTextPropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_STROKETEXTPROPERTIESDIALOG_H
