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

#ifndef LIBREPCB_EDITOR_SGI_BUSLINE_H
#define LIBREPCB_EDITOR_SGI_BUSLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/schematic/items/si_busline.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Bus;

namespace editor {

class GraphicsLayer;
class GraphicsLayerList;

/*******************************************************************************
 *  Class SGI_BusLine
 ******************************************************************************/

/**
 * @brief The SGI_BusLine class
 */
class SGI_BusLine final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SGI_BusLine() = delete;
  SGI_BusLine(const SGI_BusLine& other) = delete;
  SGI_BusLine(SI_BusLine& line, const GraphicsLayerList& layers) noexcept;
  virtual ~SGI_BusLine() noexcept;

  // General Methods
  SI_BusLine& getBusLine() noexcept { return mBusLine; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) noexcept override;

  // Operator Overloadings
  SGI_BusLine& operator=(const SGI_BusLine& rhs) = delete;

private:  // Methods
  void busLineEdited(const SI_BusLine& obj, SI_BusLine::Event event) noexcept;
  void updatePositions() noexcept;
  void updateBusName() noexcept;
  std::shared_ptr<GraphicsLayer> getLayer(const QString& name) const noexcept;

private:  // Data
  SI_BusLine& mBusLine;
  std::shared_ptr<const GraphicsLayer> mLayer;

  // Cached Attributes
  QLineF mLineF;
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  SI_BusLine::OnEditedSlot mOnBusLineEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
