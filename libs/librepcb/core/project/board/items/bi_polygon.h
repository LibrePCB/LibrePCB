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

#ifndef LIBREPCB_CORE_BI_POLYGON_H
#define LIBREPCB_CORE_BI_POLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Polygon;
class Uuid;

/*******************************************************************************
 *  Class BI_Polygon
 ******************************************************************************/

/**
 * @brief The BI_Polygon class
 */
class BI_Polygon final : public BI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_Polygon() = delete;
  BI_Polygon(const BI_Polygon& other) = delete;
  BI_Polygon(Board& board, const Polygon& polygon);
  ~BI_Polygon() noexcept;

  // Getters
  Polygon& getPolygon() noexcept { return *mPolygon; }
  const Polygon& getPolygon() const noexcept { return *mPolygon; }
  const Uuid& getUuid() const noexcept;  // for convenience, e.g. template usage

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  // Operator Overloadings
  BI_Polygon& operator=(const BI_Polygon& rhs) = delete;

private:
  QScopedPointer<Polygon> mPolygon;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
