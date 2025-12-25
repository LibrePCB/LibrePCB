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

#ifndef LIBREPCB_EDITOR_SGI_BUSJUNCTION_H
#define LIBREPCB_EDITOR_SGI_BUSJUNCTION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/schematic/items/si_busjunction.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayer;
class GraphicsLayerList;

/*******************************************************************************
 *  Class SGI_BusJunction
 ******************************************************************************/

/**
 * @brief The SGI_BusJunction class
 */
class SGI_BusJunction final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SGI_BusJunction() = delete;
  SGI_BusJunction(const SGI_BusJunction& other) = delete;
  SGI_BusJunction(SI_BusJunction& netpoint,
                  const GraphicsLayerList& layers) noexcept;
  virtual ~SGI_BusJunction() noexcept;

  // General Methods
  SI_BusJunction& getBusJunction() noexcept { return mBusJunction; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const { return sBoundingRect; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget);

  // Operator Overloadings
  SGI_BusJunction& operator=(const SGI_BusJunction& rhs) = delete;

private:  // Methods
  void busJunctionEdited(const SI_BusJunction& obj,
                         SI_BusJunction::Event event) noexcept;
  void updatePosition() noexcept;
  void updateJunction() noexcept;
  void updateToolTip() noexcept;

private:  // Data
  SI_BusJunction& mBusJunction;
  std::shared_ptr<const GraphicsLayer> mLayer;

  // Cached Attributes
  bool mIsVisibleJunction;
  bool mIsOpenLineEnd;

  // Slots
  SI_BusJunction::OnEditedSlot mOnEditedSlot;

  // Static Stuff
  static QRectF sBoundingRect;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
