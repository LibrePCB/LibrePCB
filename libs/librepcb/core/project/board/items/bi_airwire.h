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

#ifndef LIBREPCB_CORE_BI_AIRWIRE_H
#define LIBREPCB_CORE_BI_AIRWIRE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_NetLineAnchor;
class NetSignal;

/*******************************************************************************
 *  Class BI_AirWire
 ******************************************************************************/

/**
 * @brief The BI_AirWire class
 */
class BI_AirWire final : public BI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_AirWire() = delete;
  BI_AirWire(const BI_AirWire& other) = delete;
  BI_AirWire(Board& board, const NetSignal& netsignal,
             const BI_NetLineAnchor& p1, const BI_NetLineAnchor& p2);
  ~BI_AirWire() noexcept;

  // Getters
  const NetSignal& getNetSignal() const noexcept { return mNetSignal; }
  const BI_NetLineAnchor& getP1() const noexcept { return mP1; }
  const BI_NetLineAnchor& getP2() const noexcept { return mP2; }
  bool isVertical() const noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  // Operator Overloadings
  BI_AirWire& operator=(const BI_AirWire& rhs) = delete;

private:
  const NetSignal& mNetSignal;
  const BI_NetLineAnchor& mP1;
  const BI_NetLineAnchor& mP2;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
