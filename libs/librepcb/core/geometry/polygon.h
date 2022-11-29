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

#ifndef LIBREPCB_CORE_POLYGON_H
#define LIBREPCB_CORE_POLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphics/graphicslayername.h"
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"
#include "path.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Polygon
 ******************************************************************************/

/**
 * @brief The Polygon class
 */
class Polygon final {
  Q_DECLARE_TR_FUNCTIONS(Polygon)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerNameChanged,
    LineWidthChanged,
    IsFilledChanged,
    IsGrabAreaChanged,
    PathChanged,
  };
  Signal<Polygon, Event> onEdited;
  typedef Slot<Polygon, Event> OnEditedSlot;

  // Constructors / Destructor
  Polygon() = delete;
  Polygon(const Polygon& other) noexcept;
  Polygon(const Uuid& uuid, const Polygon& other) noexcept;
  Polygon(const Uuid& uuid, const GraphicsLayerName& layerName,
          const UnsignedLength& lineWidth, bool fill, bool isGrabArea,
          const Path& path) noexcept;
  Polygon(const SExpression& node, const Version& fileFormat);
  ~Polygon() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const GraphicsLayerName& getLayerName() const noexcept { return mLayerName; }
  const UnsignedLength& getLineWidth() const noexcept { return mLineWidth; }
  bool isFilled() const noexcept { return mIsFilled; }
  bool isGrabArea() const noexcept { return mIsGrabArea; }
  const Path& getPath() const noexcept { return mPath; }

  // Setters
  bool setLayerName(const GraphicsLayerName& name) noexcept;
  bool setLineWidth(const UnsignedLength& width) noexcept;
  bool setIsFilled(bool isFilled) noexcept;
  bool setIsGrabArea(bool isGrabArea) noexcept;
  bool setPath(const Path& path) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Polygon& rhs) const noexcept;
  bool operator!=(const Polygon& rhs) const noexcept { return !(*this == rhs); }
  Polygon& operator=(const Polygon& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  GraphicsLayerName mLayerName;
  UnsignedLength mLineWidth;
  bool mIsFilled;
  bool mIsGrabArea;
  Path mPath;
};

/*******************************************************************************
 *  Class PolygonList
 ******************************************************************************/

struct PolygonListNameProvider {
  static constexpr const char* tagname = "polygon";
};
using PolygonList =
    SerializableObjectList<Polygon, PolygonListNameProvider, Polygon::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
