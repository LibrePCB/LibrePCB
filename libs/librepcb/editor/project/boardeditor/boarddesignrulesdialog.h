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

#ifndef LIBREPCB_EDITOR_BOARDDESIGNRULESDIALOG_H
#define LIBREPCB_EDITOR_BOARDDESIGNRULESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/board/boarddesignrules.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;

namespace editor {

namespace Ui {
class BoardDesignRulesDialog;
}

/*******************************************************************************
 *  Class BoardDesignRulesDialog
 ******************************************************************************/

/**
 * @brief The BoardDesignRulesDialog class
 */
class BoardDesignRulesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardDesignRulesDialog() = delete;
  BoardDesignRulesDialog(const BoardDesignRulesDialog& other) = delete;
  BoardDesignRulesDialog(const BoardDesignRules& rules,
                         const LengthUnit& lengthUnit,
                         const QString& settingsPrefix, QWidget* parent = 0);
  ~BoardDesignRulesDialog();

  // Getters
  const BoardDesignRules& getDesignRules() const noexcept {
    return mDesignRules;
  }

  // Operator Overloadings
  BoardDesignRulesDialog& operator=(const BoardDesignRulesDialog& rhs) = delete;

signals:

  void rulesChanged(const BoardDesignRules& newRules);

private slots:

  void on_buttonBox_clicked(QAbstractButton* button);

private:
  void updateWidgets() noexcept;
  void applyRules() noexcept;

  Ui::BoardDesignRulesDialog* mUi;
  BoardDesignRules mDesignRules;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
