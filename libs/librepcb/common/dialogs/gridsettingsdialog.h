/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_GRIDSETTINGSDIALOG_H
#define LIBREPCB_GRIDSETTINGSDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../gridproperties.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace Ui {
class GridSettingsDialog;
}

/*******************************************************************************
 *  Class GridSettingsDialog
 ******************************************************************************/

/**
 * @brief This class provides a Dialog (GUI) to change the grid settings of a
 * #GraphicsView
 *
 * @author ubruhin
 * @date 2014-10-13
 */
class GridSettingsDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit GridSettingsDialog(const GridProperties& grid, QWidget* parent = 0);
  ~GridSettingsDialog();

  // Getters
  const GridProperties& getGrid() const noexcept { return mCurrentGrid; }

private slots:

  // Private Slots
  void rbtnGroupClicked(int id);
  void spbxIntervalChanged(double value);
  void cbxUnitsChanged(int index);
  void btnMul2Clicked();
  void btnDiv2Clicked();
  void buttonBoxClicked(QAbstractButton* button);

signals:

  void gridPropertiesChanged(const GridProperties& grid);

private:
  // make some methods inaccessible...
  GridSettingsDialog();
  GridSettingsDialog(const GridSettingsDialog& other);
  GridSettingsDialog& operator=(const GridSettingsDialog& rhs);

  // Private Methods
  void updateInternalRepresentation() noexcept;

  // General Attributes
  Ui::GridSettingsDialog* mUi;
  GridProperties          mOriginalGrid;
  GridProperties          mCurrentGrid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_GRIDSETTINGSDIALOG_H
