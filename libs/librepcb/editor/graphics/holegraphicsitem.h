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

#ifndef LIBREPCB_EDITOR_HOLEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_HOLEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/hole.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;
class PrimitiveHoleGraphicsItem;

/*******************************************************************************
 *  Class HoleGraphicsItem
 ******************************************************************************/

/**
 * @brief The HoleGraphicsItem class is the graphical representation of a
 *        librepcb::Hole
 */
class HoleGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  HoleGraphicsItem() = delete;
  HoleGraphicsItem(const HoleGraphicsItem& other) = delete;
  HoleGraphicsItem(Hole& hole, const GraphicsLayerList& layers,
                   bool originCrossesVisible,
                   QGraphicsItem* parent = nullptr) noexcept;
  virtual ~HoleGraphicsItem() noexcept;

  // Getters
  Hole& getObj() noexcept { return mHole; }

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  HoleGraphicsItem& operator=(const HoleGraphicsItem& rhs) = delete;

private:  // Methods
  void holeEdited(const Hole& hole, Hole::Event event) noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void updateHole() noexcept;

private:  // Data
  Hole& mHole;
  QScopedPointer<PrimitiveHoleGraphicsItem> mGraphicsItem;

  // Slots
  Hole::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
