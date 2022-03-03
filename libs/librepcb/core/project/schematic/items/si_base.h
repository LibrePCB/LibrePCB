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
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;
class GraphicsScene;
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
  // Types
  enum class Type_t {
    NetSegment,  ///< ::librepcb::SI_NetSegment
    NetPoint,  ///< ::librepcb::SI_NetPoint
    NetLine,  ///< ::librepcb::SI_NetLine
    NetLabel,  ///< ::librepcb::SI_NetLabel
    Symbol,  ///< ::librepcb::SI_Symbol
    SymbolPin,  ///< ::librepcb::SI_SymbolPin
    Polygon,  ///< ::librepcb::SI_Polygon
    Text,  ///< ::librepcb::SI_Text
  };

  // Constructors / Destructor
  SI_Base() = delete;
  SI_Base(const SI_Base& other) = delete;
  SI_Base(Schematic& schematic) noexcept;
  virtual ~SI_Base() noexcept;

  // Getters
  Project& getProject() const noexcept;
  Circuit& getCircuit() const noexcept;
  Schematic& getSchematic() const noexcept { return mSchematic; }
  virtual Type_t getType() const noexcept = 0;
  virtual QPainterPath getGrabAreaScenePx() const noexcept = 0;
  virtual bool isAddedToSchematic() const noexcept {
    return mIsAddedToSchematic;
  }
  virtual bool isSelected() const noexcept { return mIsSelected; }

  // Setters
  virtual void setSelected(bool selected) noexcept;

  // General Methods
  virtual void addToSchematic() = 0;
  virtual void removeFromSchematic() = 0;

  // Operator Overloadings
  SI_Base& operator=(const SI_Base& rhs) = delete;

protected:
  // General Methods
  void addToSchematic(QGraphicsItem* item) noexcept;
  void removeFromSchematic(QGraphicsItem* item) noexcept;

protected:
  Schematic& mSchematic;

private:
  // General Attributes
  bool mIsAddedToSchematic;
  bool mIsSelected;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
