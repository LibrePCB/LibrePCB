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

#ifndef LIBREPCB_PROJECT_BGI_BASE_H
#define LIBREPCB_PROJECT_BGI_BASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../board.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Class BGI_Base
 ******************************************************************************/

/**
 * @brief The Board Graphics Item Base (BGI_Base) class
 */
class BGI_Base : public QGraphicsItem {
public:
  // Constructors / Destructor
  explicit BGI_Base() noexcept;
  BGI_Base(const BGI_Base& other) = delete;
  virtual ~BGI_Base() noexcept;

  // Operator Overloadings
  BGI_Base& operator=(const BGI_Base& rhs) = delete;

protected:
  static qreal getZValueOfCopperLayer(const QString& name) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
