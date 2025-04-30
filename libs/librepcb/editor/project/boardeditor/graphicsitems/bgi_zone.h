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

#ifndef LIBREPCB_EDITOR_BGI_ZONE_H
#define LIBREPCB_EDITOR_BGI_ZONE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/project/board/items/bi_zone.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;
class PrimitiveZoneGraphicsItem;

/*******************************************************************************
 *  Class BGI_Zone
 ******************************************************************************/

/**
 * @brief The BGI_Zone class
 */
class BGI_Zone final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  BGI_Zone() = delete;
  BGI_Zone(const BGI_Zone& other) = delete;
  BGI_Zone(BI_Zone& zone, const GraphicsLayerList& layers) noexcept;
  virtual ~BGI_Zone() noexcept;

  // General Methods
  BI_Zone& getZone() noexcept { return mZone; }
  const PrimitiveZoneGraphicsItem& getGraphicsItem() const noexcept {
    return *mGraphicsItem;
  }

  /// Get the line segment at a specific position
  ///
  /// @param pos    The position to check for lines.
  /// @return       The index of the vertex *after* the line under the cursor.
  ///               So for the first line segment, index 1 is returned. If no
  ///               line is located under the specified position, -1 is
  ///               returned.
  int getLineIndexAtPosition(const Point& pos) const noexcept;

  /// Get the vertices at a specific position
  ///
  /// @param pos    The position to check for vertices.
  /// @return       All indices of the vertices at the specified position.
  QVector<int> getVertexIndicesAtPosition(const Point& pos) const noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  BGI_Zone& operator=(const BGI_Zone& rhs) = delete;

private:  // Methods
  void zoneEdited(const BI_Zone& obj, BI_Zone::Event event) noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void updateZValue() noexcept;
  void updateEditable() noexcept;

private:  // Data
  BI_Zone& mZone;
  QScopedPointer<PrimitiveZoneGraphicsItem> mGraphicsItem;

  // Slots
  BI_Zone::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
