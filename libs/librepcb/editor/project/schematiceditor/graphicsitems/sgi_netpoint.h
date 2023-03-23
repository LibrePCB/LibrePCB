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

#ifndef LIBREPCB_EDITOR_SGI_NETPOINT_H
#define LIBREPCB_EDITOR_SGI_NETPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/schematic/items/si_netpoint.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayer;
class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class SGI_NetPoint
 ******************************************************************************/

/**
 * @brief The SGI_NetPoint class
 */
class SGI_NetPoint final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SGI_NetPoint() = delete;
  SGI_NetPoint(const SGI_NetPoint& other) = delete;
  SGI_NetPoint(SI_NetPoint& netpoint, const IF_GraphicsLayerProvider& lp,
               std::shared_ptr<const QSet<const NetSignal*>>
                   highlightedNetSignals) noexcept;
  virtual ~SGI_NetPoint() noexcept;

  // General Methods
  SI_NetPoint& getNetPoint() noexcept { return mNetPoint; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const { return sBoundingRect; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget);

  // Operator Overloadings
  SGI_NetPoint& operator=(const SGI_NetPoint& rhs) = delete;

private:  // Methods
  void netPointEdited(const SI_NetPoint& obj,
                      SI_NetPoint::Event event) noexcept;
  void updatePosition() noexcept;
  void updateJunction() noexcept;
  void updateNetName() noexcept;

private:  // Data
  SI_NetPoint& mNetPoint;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  GraphicsLayer* mLayer;

  // Cached Attributes
  bool mIsVisibleJunction;
  bool mIsOpenLineEnd;

  // Slots
  SI_NetPoint::OnEditedSlot mOnEditedSlot;

  // Static Stuff
  static QRectF sBoundingRect;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
