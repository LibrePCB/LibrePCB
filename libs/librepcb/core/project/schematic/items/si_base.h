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

#ifndef LIBREPCB_CORE_SI_BASE_H
#define LIBREPCB_CORE_SI_BASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class Point;
class Project;
class Schematic;

/*******************************************************************************
 *  Class SI_Base
 ******************************************************************************/

/**
 * @brief The Schematic Item Base (SI_Base) class
 */
class SI_Base : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_Base() = delete;
  SI_Base(const SI_Base& other) = delete;
  SI_Base(Schematic& schematic) noexcept;
  virtual ~SI_Base() noexcept;

  // Getters
  Project& getProject() const noexcept;
  Circuit& getCircuit() const noexcept;
  Schematic& getSchematic() const noexcept { return mSchematic; }
  virtual bool isAddedToSchematic() const noexcept {
    return mIsAddedToSchematic;
  }

  // General Methods
  virtual void addToSchematic();
  virtual void removeFromSchematic();

  // Operator Overloadings
  SI_Base& operator=(const SI_Base& rhs) = delete;

protected:
  Schematic& mSchematic;

private:
  // General Attributes
  bool mIsAddedToSchematic;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
