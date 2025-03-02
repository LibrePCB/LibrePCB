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

#ifndef LIBREPCB_EDITOR_BGI_STROKETEXT_H
#define LIBREPCB_EDITOR_BGI_STROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bgi_device.h"

#include <librepcb/core/project/board/items/bi_stroketext.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class IF_GraphicsLayerProvider;
class LineGraphicsItem;
class OriginCrossGraphicsItem;
class PrimitivePathGraphicsItem;

/*******************************************************************************
 *  Class BGI_StrokeText
 ******************************************************************************/

/**
 * @brief The BGI_StrokeText class
 */
class BGI_StrokeText final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  BGI_StrokeText() = delete;
  BGI_StrokeText(const BGI_StrokeText& other) = delete;
  BGI_StrokeText(BI_StrokeText& text, std::weak_ptr<BGI_Device> deviceItem,
                 const IF_GraphicsLayerProvider& lp) noexcept;
  virtual ~BGI_StrokeText() noexcept;

  // General Methods
  BI_StrokeText& getStrokeText() noexcept { return mText; }
  const std::weak_ptr<BGI_Device>& getDeviceGraphicsItem() noexcept {
    return mDeviceGraphicsItem;
  }

  // Inherited from QGraphicsItem
  virtual QPainterPath shape() const noexcept override;

  // Operator Overloadings
  BGI_StrokeText& operator=(const BGI_StrokeText& rhs) = delete;

private:  // Methods
  void strokeTextEdited(const BI_StrokeText& obj,
                        BI_StrokeText::Event event) noexcept;
  void deviceGraphicsItemEdited(const BGI_Device& obj,
                                BGI_Device::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updatePosition() noexcept;
  void updateTransform() noexcept;
  void updateLayer() noexcept;
  void updateStrokeWidth() noexcept;
  void updatePaths() noexcept;
  void updateAnchorLayer() noexcept;
  void updateAnchorLine() noexcept;

private:  // Data
  BI_StrokeText& mText;
  std::weak_ptr<BGI_Device> mDeviceGraphicsItem;
  const IF_GraphicsLayerProvider& mLayerProvider;
  QScopedPointer<PrimitivePathGraphicsItem> mPathGraphicsItem;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;

  // Slots
  BI_StrokeText::OnEditedSlot mOnEditedSlot;
  BGI_Device::OnEditedSlot mOnDeviceEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
