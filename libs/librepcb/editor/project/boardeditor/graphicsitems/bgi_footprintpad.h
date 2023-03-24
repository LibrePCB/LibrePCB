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

#ifndef LIBREPCB_EDITOR_BGI_FOOTPRINTPAD_H
#define LIBREPCB_EDITOR_BGI_FOOTPRINTPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bgi_device.h"

#include <librepcb/core/project/board/items/bi_footprintpad.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;

namespace editor {

class IF_GraphicsLayerProvider;
class PrimitiveFootprintPadGraphicsItem;

/*******************************************************************************
 *  Class BGI_FootprintPad
 ******************************************************************************/

/**
 * @brief The BGI_FootprintPad class
 */
class BGI_FootprintPad final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  BGI_FootprintPad() = delete;
  BGI_FootprintPad(const BGI_FootprintPad& other) = delete;
  BGI_FootprintPad(BI_FootprintPad& pad, std::weak_ptr<BGI_Device> deviceItem,
                   const IF_GraphicsLayerProvider& lp,
                   std::shared_ptr<const QSet<const NetSignal*>>
                       highlightedNetSignals) noexcept;
  virtual ~BGI_FootprintPad() noexcept;

  // General Methods
  BI_FootprintPad& getPad() noexcept { return mPad; }
  const std::weak_ptr<BGI_Device>& getDeviceGraphicsItem() noexcept {
    return mDeviceGraphicsItem;
  }

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  BGI_FootprintPad& operator=(const BGI_FootprintPad& rhs) = delete;

private:  // Methods
  void padEdited(const BI_FootprintPad& obj,
                 BI_FootprintPad::Event event) noexcept;
  void deviceGraphicsItemEdited(const BGI_Device& obj,
                                BGI_Device::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updateLayer() noexcept;

private:  // Data
  BI_FootprintPad& mPad;
  std::weak_ptr<BGI_Device> mDeviceGraphicsItem;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  QScopedPointer<PrimitiveFootprintPadGraphicsItem> mGraphicsItem;

  // Slots
  BI_FootprintPad::OnEditedSlot mOnPadEditedSlot;
  BGI_Device::OnEditedSlot mOnDeviceEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
