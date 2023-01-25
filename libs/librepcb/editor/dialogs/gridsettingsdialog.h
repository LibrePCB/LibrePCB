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

#ifndef LIBREPCB_EDITOR_GRIDSETTINGSDIALOG_H
#define LIBREPCB_EDITOR_GRIDSETTINGSDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class GridSettingsDialog;
}

/*******************************************************************************
 *  Class GridSettingsDialog
 ******************************************************************************/

/**
 * @brief This class provides a Dialog (GUI) to change the grid settings of a
 * ::librepcb::editor::GraphicsView
 */
class GridSettingsDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  GridSettingsDialog() = delete;
  GridSettingsDialog(const GridSettingsDialog& other) = delete;
  explicit GridSettingsDialog(const PositiveLength& interval,
                              const LengthUnit& unit, Theme::GridStyle style,
                              QWidget* parent = nullptr) noexcept;
  ~GridSettingsDialog() noexcept;

  // Getters
  const PositiveLength& getInterval() const noexcept {
    return mCurrentGrid.interval;
  }
  const LengthUnit& getUnit() const noexcept { return mCurrentGrid.unit; }
  Theme::GridStyle getStyle() const noexcept { return mCurrentGrid.style; }

  // Operator Overloadings
  GridSettingsDialog& operator=(const GridSettingsDialog& rhs) = delete;

signals:
  void gridPropertiesChanged(const PositiveLength& interval,
                             const LengthUnit& unit, Theme::GridStyle style);

private:  // Methods
  void rbtnGroupClicked(int id) noexcept;
  void edtIntervalValueChanged(const PositiveLength& value) noexcept;
  void edtIntervalUnitChanged(const LengthUnit& unit) noexcept;
  void buttonBoxClicked(QAbstractButton* button) noexcept;

private:  // Data
  struct Grid {
    PositiveLength interval;
    LengthUnit unit;
    Theme::GridStyle style;
  };

  QScopedPointer<Ui::GridSettingsDialog> mUi;
  Grid mOriginalGrid;
  Grid mCurrentGrid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
