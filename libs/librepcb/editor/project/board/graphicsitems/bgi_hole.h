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

#ifndef LIBREPCB_EDITOR_BGI_HOLE_H
#define LIBREPCB_EDITOR_BGI_HOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/board/items/bi_hole.h>

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
 *  Class BGI_Hole
 ******************************************************************************/

/**
 * @brief The BGI_Hole class
 */
class BGI_Hole final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  BGI_Hole() = delete;
  BGI_Hole(const BGI_Hole& other) = delete;
  BGI_Hole(BI_Hole& hole, const GraphicsLayerList& layers) noexcept;
  virtual ~BGI_Hole() noexcept;

  // General Methods
  BI_Hole& getHole() noexcept { return mHole; }

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  BGI_Hole& operator=(const BGI_Hole& rhs) = delete;

private:  // Methods
  void holeEdited(const BI_Hole& obj, BI_Hole::Event event) noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void updateHole() noexcept;

private:  // Data
  BI_Hole& mHole;
  QScopedPointer<PrimitiveHoleGraphicsItem> mGraphicsItem;

  // Slots
  BI_Hole::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
