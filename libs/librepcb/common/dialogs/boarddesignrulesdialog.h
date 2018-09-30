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

#ifndef LIBREPCB_BOARDDESIGNRULESDIALOG_H
#define LIBREPCB_BOARDDESIGNRULESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../boarddesignrules.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace Ui {
class BoardDesignRulesDialog;
}

/*******************************************************************************
 *  Class BoardDesignRulesDialog
 ******************************************************************************/

/**
 * @brief The BoardDesignRulesDialog class
 *
 * @author ubruhin
 * @date 2016-04-01
 */
class BoardDesignRulesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardDesignRulesDialog()                                    = delete;
  BoardDesignRulesDialog(const BoardDesignRulesDialog& other) = delete;
  explicit BoardDesignRulesDialog(const BoardDesignRules& rules,
                                  QWidget*                parent = 0);
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
  BoardDesignRules            mDesignRules;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_BOARDDESIGNRULESDIALOG_H
