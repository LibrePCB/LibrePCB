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

#ifndef LIBREPCB_POLYGON_H
#define LIBREPCB_POLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../fileio/serializableobjectlist.h"
#include "../graphics/graphicslayername.h"
#include "../units/all_length_units.h"
#include "path.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Interface IF_PolygonObserver
 ******************************************************************************/

/**
 * @brief The IF_PolygonObserver class
 *
 * @author ubruhin
 * @date 2017-01-05
 */
class IF_PolygonObserver {
public:
  virtual void polygonLayerNameChanged(
      const GraphicsLayerName& newLayerName) noexcept = 0;
  virtual void polygonLineWidthChanged(
      const UnsignedLength& newLineWidth) noexcept                   = 0;
  virtual void polygonIsFilledChanged(bool newIsFilled) noexcept     = 0;
  virtual void polygonIsGrabAreaChanged(bool newIsGrabArea) noexcept = 0;
  virtual void polygonPathChanged(const Path& newPath) noexcept      = 0;

protected:
  IF_PolygonObserver() noexcept {}
  explicit IF_PolygonObserver(const IF_PolygonObserver& other) = delete;
  virtual ~IF_PolygonObserver() noexcept {}
  IF_PolygonObserver& operator=(const IF_PolygonObserver& rhs) = delete;
};

/*******************************************************************************
 *  Class Polygon
 ******************************************************************************/

/**
 * @brief The Polygon class
 */
class Polygon final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Polygon)

public:
  // Constructors / Destructor
  Polygon() = delete;
  Polygon(const Polygon& other) noexcept;
  Polygon(const Uuid& uuid, const Polygon& other) noexcept;
  Polygon(const Uuid& uuid, const GraphicsLayerName& layerName,
          const UnsignedLength& lineWidth, bool fill, bool isGrabArea,
          const Path& path) noexcept;
  explicit Polygon(const SExpression& node);
  ~Polygon() noexcept;

  // Getters
  const Uuid&              getUuid() const noexcept { return mUuid; }
  const GraphicsLayerName& getLayerName() const noexcept { return mLayerName; }
  const UnsignedLength&    getLineWidth() const noexcept { return mLineWidth; }
  bool                     isFilled() const noexcept { return mIsFilled; }
  bool                     isGrabArea() const noexcept { return mIsGrabArea; }
  const Path&              getPath() const noexcept { return mPath; }

  // Setters
  void setLayerName(const GraphicsLayerName& name) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void setIsFilled(bool isFilled) noexcept;
  void setIsGrabArea(bool isGrabArea) noexcept;
  void setPath(const Path& path) noexcept;

  // General Methods
  void registerObserver(IF_PolygonObserver& object) const noexcept;
  void unregisterObserver(IF_PolygonObserver& object) const noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Polygon& rhs) const noexcept;
  bool operator!=(const Polygon& rhs) const noexcept { return !(*this == rhs); }
  Polygon& operator=(const Polygon& rhs) noexcept;

private:  // Data
  Uuid              mUuid;
  GraphicsLayerName mLayerName;
  UnsignedLength    mLineWidth;
  bool              mIsFilled;
  bool              mIsGrabArea;
  Path              mPath;

  // Misc
  mutable QSet<IF_PolygonObserver*>
      mObservers;  ///< A list of all observer objects
};

/*******************************************************************************
 *  Class PolygonList
 ******************************************************************************/

struct PolygonListNameProvider {
  static constexpr const char* tagname = "polygon";
};
using PolygonList = SerializableObjectList<Polygon, PolygonListNameProvider>;
using CmdPolygonInsert = CmdListElementInsert<Polygon, PolygonListNameProvider>;
using CmdPolygonRemove = CmdListElementRemove<Polygon, PolygonListNameProvider>;
using CmdPolygonsSwap  = CmdListElementsSwap<Polygon, PolygonListNameProvider>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_POLYGON_H
