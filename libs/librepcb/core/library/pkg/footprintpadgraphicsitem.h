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

#ifndef LIBREPCB_CORE_FOOTPRINTPADGRAPHICSITEM_H
#define LIBREPCB_CORE_FOOTPRINTPADGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../pkg/packagepad.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class FootprintPad;
class IF_GraphicsLayerProvider;
class Point;
class PrimitivePathGraphicsItem;
class PrimitiveTextGraphicsItem;

/*******************************************************************************
 *  Class FootprintPadGraphicsItem
 ******************************************************************************/

/**
 * @brief The FootprintPadGraphicsItem class
 */
class FootprintPadGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  FootprintPadGraphicsItem() = delete;
  FootprintPadGraphicsItem(const FootprintPadGraphicsItem& other) = delete;
  FootprintPadGraphicsItem(FootprintPad& pad,
                           const IF_GraphicsLayerProvider& lp,
                           const PackagePadList* packagePadList,
                           QGraphicsItem* parent = nullptr) noexcept;
  ~FootprintPadGraphicsItem() noexcept;

  // Getters
  FootprintPad& getPad() noexcept { return mPad; }

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setShape(const QPainterPath& shape) noexcept;
  void setLayerName(const QString& name) noexcept;
  void setPackagePadUuid(const Uuid& uuid) noexcept;
  void setSelected(bool selected) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return QRectF(); }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  FootprintPadGraphicsItem& operator=(const FootprintPadGraphicsItem& rhs) =
      delete;

private:  // Methods
  void packagePadListEdited(const PackagePadList& list, int index,
                            const std::shared_ptr<const PackagePad>& pad,
                            PackagePadList::Event event) noexcept;
  void updateTextHeight() noexcept;

private:  // Data
  FootprintPad& mPad;
  const IF_GraphicsLayerProvider& mLayerProvider;
  const PackagePadList* mPackagePadList;
  QScopedPointer<PrimitivePathGraphicsItem> mPathGraphicsItem;
  QScopedPointer<PrimitiveTextGraphicsItem> mTextGraphicsItem;

  // Slots
  PackagePadList::OnEditedSlot mOnPadsEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
