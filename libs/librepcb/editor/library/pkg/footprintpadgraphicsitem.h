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

#ifndef LIBREPCB_EDITOR_FOOTPRINTPADGRAPHICSITEM_H
#define LIBREPCB_EDITOR_FOOTPRINTPADGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;
class PrimitiveFootprintPadGraphicsItem;

/*******************************************************************************
 *  Class FootprintPadGraphicsItem
 ******************************************************************************/

/**
 * @brief The FootprintPadGraphicsItem class
 */
class FootprintPadGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  FootprintPadGraphicsItem() = delete;
  FootprintPadGraphicsItem(const FootprintPadGraphicsItem& other) = delete;
  FootprintPadGraphicsItem(std::shared_ptr<FootprintPad> pad,
                           const GraphicsLayerList& layers,
                           const PackagePadList* packagePadList,
                           QGraphicsItem* parent = nullptr) noexcept;
  ~FootprintPadGraphicsItem() noexcept;

  // Getters
  FootprintPad& getObj() noexcept { return *mPad; }

  // General Methods
  void updateText() noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  FootprintPadGraphicsItem& operator=(const FootprintPadGraphicsItem& rhs) =
      delete;

private:  // Methods
  void padEdited(const FootprintPad& pad, FootprintPad::Event event) noexcept;
  void packagePadListEdited(const PackagePadList& list, int index,
                            const std::shared_ptr<const PackagePad>& pad,
                            PackagePadList::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updateLayer() noexcept;
  void updateGeometries() noexcept;

private:  // Data
  std::shared_ptr<FootprintPad> mPad;
  const PackagePadList* mPackagePadList;
  QScopedPointer<PrimitiveFootprintPadGraphicsItem> mGraphicsItem;

  // Slots
  FootprintPad::OnEditedSlot mOnPadEditedSlot;
  PackagePadList::OnEditedSlot mOnPackagePadsEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
