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

#ifndef BOARDDESIGNRULECHECKMESSAGESDOCK_H
#define BOARDDESIGNRULECHECKMESSAGESDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/project/boards/drc/boarddesignrulecheckmessage.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

namespace Ui {
class BoardDesignRuleCheckMessagesDock;
}

/*******************************************************************************
 *  Class BoardDesignRuleCheckMessagesDock
 ******************************************************************************/

/**
 * @brief The BoardDesignRuleCheckMessagesDock class
 */
class BoardDesignRuleCheckMessagesDock final : public QDockWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BoardDesignRuleCheckMessagesDock(QWidget* parent = nullptr) noexcept;
  BoardDesignRuleCheckMessagesDock(
      const BoardDesignRuleCheckMessagesDock& other) = delete;
  ~BoardDesignRuleCheckMessagesDock() noexcept;

  // Setters
  void setMessages(const QList<BoardDesignRuleCheckMessage>& messages) noexcept;

  // Operator Overloadings
  BoardDesignRuleCheckMessagesDock& operator       =(
      const BoardDesignRuleCheckMessagesDock& rhs) = delete;

signals:
  void messageSelected(const BoardDesignRuleCheckMessage& msg, bool zoomTo);

private:  // Methods
  void listWidgetCurrentItemChanged() noexcept;
  void listWidgetCurrentItemDoubleClicked() noexcept;

private:
  QScopedPointer<Ui::BoardDesignRuleCheckMessagesDock> mUi;
  QList<BoardDesignRuleCheckMessage>                   mMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // BOARDDESIGNRULECHECKMESSAGESDOCK_H
