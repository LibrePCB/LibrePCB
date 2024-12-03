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

#ifndef LIBREPCB_EDITOR_PRIMITIVEHOLEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_PRIMITIVEHOLEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/path.h>
#include <librepcb/core/types/length.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayer;
class IF_GraphicsLayerProvider;
class OriginCrossGraphicsItem;
class PrimitivePathGraphicsItem;

/*******************************************************************************
 *  Class PrimitiveHoleGraphicsItem
 ******************************************************************************/

/**
 * @brief Independent graphical representation of a ::librepcb::Hole
 */
class PrimitiveHoleGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  PrimitiveHoleGraphicsItem() = delete;
  PrimitiveHoleGraphicsItem(const PrimitiveHoleGraphicsItem& other) = delete;
  PrimitiveHoleGraphicsItem(const IF_GraphicsLayerProvider& lp,
                            bool originCrossesVisible,
                            QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PrimitiveHoleGraphicsItem() noexcept;

  // Setters
  void setHole(const NonEmptyPath& path, const PositiveLength& diameter,
               const std::optional<Length>& stopMaskOffset) noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  PrimitiveHoleGraphicsItem& operator=(const PrimitiveHoleGraphicsItem& rhs) =
      delete;

private:  // Methods
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;

private:  // Data
  std::shared_ptr<GraphicsLayer> mHoleLayer;
  QScopedPointer<PrimitivePathGraphicsItem> mHoleGraphicsItem;
  QScopedPointer<PrimitivePathGraphicsItem> mStopMaskGraphicsItemBot;
  QScopedPointer<PrimitivePathGraphicsItem> mStopMaskGraphicsItemTop;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItemStart;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItemEnd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
