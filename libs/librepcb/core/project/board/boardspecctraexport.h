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

#ifndef LIBREPCB_CORE_BOARDSPECCTRAEXPORT_H
#define LIBREPCB_CORE_BOARDSPECCTRAEXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

/*******************************************************************************
 *  Class BoardSpecctraExport
 ******************************************************************************/

/**
 * @brief Specctra DSN Export
 */
class BoardSpecctraExport final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardSpecctraExport() = delete;
  BoardSpecctraExport(const BoardSpecctraExport& other) = delete;
  explicit BoardSpecctraExport(const Board& board) noexcept;
  ~BoardSpecctraExport() noexcept;

  // General Methods
  QByteArray generate() const;

  // Operator Overloadings
  BoardSpecctraExport& operator=(const BoardSpecctraExport& rhs) = delete;

private:
  // Private Member Variables
  const Board& mBoard;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
