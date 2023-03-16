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

#ifndef LIBREPCB_CORE_BOARDD356NETLISTEXPORT_H
#define LIBREPCB_CORE_BOARDD356NETLISTEXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class FilePath;

/*******************************************************************************
 *  Class BoardD356NetlistExport
 ******************************************************************************/

/**
 * @brief The BoardD356NetlistExport class
 */
class BoardD356NetlistExport final {
public:
  // Constructors / Destructor
  BoardD356NetlistExport() = delete;
  BoardD356NetlistExport(const BoardD356NetlistExport& other) = delete;
  explicit BoardD356NetlistExport(const Board& board) noexcept;
  ~BoardD356NetlistExport() noexcept;

  // General Methods
  QByteArray generate() const;

  // Operator Overloadings
  BoardD356NetlistExport& operator=(const BoardD356NetlistExport& rhs) = delete;

private:
  const Board& mBoard;
  QDateTime mCreationDateTime;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
