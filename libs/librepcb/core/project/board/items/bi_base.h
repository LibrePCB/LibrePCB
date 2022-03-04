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

#ifndef LIBREPCB_CORE_BI_BASE_H
#define LIBREPCB_CORE_BI_BASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BGI_Base;
class Board;
class Circuit;
class GraphicsScene;
class Point;
class Project;

/*******************************************************************************
 *  Class BI_Base
 ******************************************************************************/

/**
 * @brief The Board Item Base (BI_Base) class
 */
class BI_Base : public QObject {
  Q_OBJECT

public:
  // Types
  enum class Type_t {
    NetSegment,  ///< ::librepcb::BI_NetSegment
    NetPoint,  ///< ::librepcb::BI_NetPoint
    NetLine,  ///< ::librepcb::BI_NetLine
    Via,  ///< ::librepcb::BI_Via
    Device,  ///< ::librepcb::BI_Device
    Footprint,  ///< ::librepcb::BI_Footprint
    FootprintPad,  ///< ::librepcb::BI_FootprintPad
    Polygon,  ///< ::librepcb::BI_Polygon
    StrokeText,  ///< ::librepcb::BI_StrokeText
    Hole,  ///< ::librepcb::BI_Hole
    Plane,  ///< ::librepcb::BI_Plane
    AirWire,  ///< ::librepcb::BI_AirWire
  };

  // Constructors / Destructor
  BI_Base() = delete;
  BI_Base(const BI_Base& other) = delete;
  BI_Base(Board& board) noexcept;
  virtual ~BI_Base() noexcept;

  // Getters
  Project& getProject() const noexcept;
  Circuit& getCircuit() const noexcept;
  Board& getBoard() const noexcept { return mBoard; }
  virtual Type_t getType() const noexcept = 0;
  virtual QPainterPath getGrabAreaScenePx() const noexcept = 0;
  virtual bool isAddedToBoard() const noexcept { return mIsAddedToBoard; }
  virtual bool isSelectable() const noexcept = 0;
  virtual bool isSelected() const noexcept { return mIsSelected; }

  // Setters
  virtual void setSelected(bool selected) noexcept;

  // General Methods
  virtual void addToBoard() = 0;
  virtual void removeFromBoard() = 0;

  // Operator Overloadings
  BI_Base& operator=(const BI_Base& rhs) = delete;

protected:
  // General Methods
  void addToBoard(QGraphicsItem* item) noexcept;
  void removeFromBoard(QGraphicsItem* item) noexcept;

protected:
  Board& mBoard;

private:
  // General Attributes
  bool mIsAddedToBoard;
  bool mIsSelected;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
