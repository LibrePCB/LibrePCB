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

#ifndef LIBREPCB_PROJECTEDITOR_BOARDDESIGNRULECHECKDIALOG_H
#define LIBREPCB_PROJECTEDITOR_BOARDDESIGNRULECHECKDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/project/boards/drc/boarddesignrulecheck.h>

#include <QtCore>
#include <QtWidgets>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;

namespace project {

class Board;

namespace editor {

namespace Ui {
class BoardDesignRuleCheckDialog;
}

/*******************************************************************************
 *  Class BoardDesignRuleCheckDialog
 ******************************************************************************/

/**
 * @brief The BoardDesignRuleCheckDialog class
 */
class BoardDesignRuleCheckDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardDesignRuleCheckDialog() = delete;
  BoardDesignRuleCheckDialog(const BoardDesignRuleCheckDialog& other) = delete;
  BoardDesignRuleCheckDialog(Board& board,
                             const BoardDesignRuleCheck::Options& options,
                             const LengthUnit& lengthUnit,
                             const QString& settingsPrefix,
                             QWidget* parent = 0) noexcept;
  ~BoardDesignRuleCheckDialog();

  // Getters
  BoardDesignRuleCheck::Options getOptions() const noexcept;
  const tl::optional<QList<BoardDesignRuleCheckMessage>>& getMessages() const
      noexcept {
    return mMessages;
  }

private:  // GUI Event Handlers
  void btnRunDrcClicked() noexcept;

private:
  Board& mBoard;
  QScopedPointer<Ui::BoardDesignRuleCheckDialog> mUi;
  tl::optional<QList<BoardDesignRuleCheckMessage>> mMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
