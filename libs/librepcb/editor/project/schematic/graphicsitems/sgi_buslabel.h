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

#ifndef LIBREPCB_EDITOR_SGI_BUSLABEL_H
#define LIBREPCB_EDITOR_SGI_BUSLABEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/schematic/items/si_buslabel.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class LineGraphicsItem;
class OriginCrossGraphicsItem;
class PrimitiveTextGraphicsItem;

/*******************************************************************************
 *  Class SGI_BusLabel
 ******************************************************************************/

/**
 * @brief The SGI_BusLabel class
 */
class SGI_BusLabel final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  SGI_BusLabel() = delete;
  SGI_BusLabel(const SGI_BusLabel& other) = delete;
  SGI_BusLabel(
      SI_BusLabel& label, const GraphicsLayerList& layers,
      std::shared_ptr<const SchematicGraphicsScene::Context> context) noexcept;
  virtual ~SGI_BusLabel() noexcept;

  // General Methods
  SI_BusLabel& getBusLabel() noexcept { return mLabel; }
  void updateContext() noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  SGI_BusLabel& operator=(const SGI_BusLabel& rhs) = delete;

private:  // Methods
  void labelEdited(const SI_BusLabel& obj, SI_BusLabel::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updatePosition() noexcept;
  void updateRotation() noexcept;
  void updateMirrored() noexcept;
  void updateText() noexcept;
  void updateAnchor() noexcept;

private:  // Data
  SI_BusLabel& mLabel;
  std::shared_ptr<const SchematicGraphicsScene::Context> mContext;
  std::unique_ptr<PrimitiveTextGraphicsItem> mTextGraphicsItem;
  std::unique_ptr<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  std::unique_ptr<LineGraphicsItem> mAnchorGraphicsItem;

  // Slots
  SI_BusLabel::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
