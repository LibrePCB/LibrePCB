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

#ifndef LIBREPCB_CORE_VIA_H
#define LIBREPCB_CORE_VIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"
#include "../types/maskconfig.h"
#include "../types/point.h"
#include "path.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class Via
 ******************************************************************************/

/**
 * @brief The Via class represents a via of a board
 *
 * The main purpose of this class is to serialize and deserialize vias
 * contained in boards.
 */
class Via final {
  Q_DECLARE_TR_FUNCTIONS(Via)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayersChanged,
    PositionChanged,
    SizeChanged,
    DrillDiameterChanged,
    ExposureConfigChanged,
  };
  Signal<Via, Event> onEdited;
  typedef Slot<Via, Event> OnEditedSlot;

  // Constructors / Destructor
  Via() = delete;
  Via(const Via& other) noexcept;
  Via(const Uuid& uuid, const Via& other) noexcept;
  Via(const Uuid& uuid, const Layer& startLayer, const Layer& endLayer,
      const Point& position, const PositiveLength& size,
      const PositiveLength& drillDiameter,
      const MaskConfig& exposureConfig) noexcept;
  explicit Via(const SExpression& node);
  ~Via() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Layer& getStartLayer() const noexcept { return *mStartLayer; }
  const Layer& getEndLayer() const noexcept { return *mEndLayer; }
  const Point& getPosition() const noexcept { return mPosition; }
  const PositiveLength& getSize() const noexcept { return mSize; }
  const PositiveLength& getDrillDiameter() const noexcept {
    return mDrillDiameter;
  }
  const MaskConfig& getExposureConfig() const noexcept {
    return mExposureConfig;
  }
  Path getOutline(const Length& expansion = Length(0)) const noexcept;
  Path getSceneOutline(const Length& expansion = Length(0)) const noexcept;
  bool isThrough() const noexcept;
  bool isBlind() const noexcept;
  bool isBuried() const noexcept;
  bool isOnLayer(const Layer& layer) const noexcept;
  bool isOnAnyLayer(const QSet<const Layer*>& layers) const noexcept;
  QPainterPath toQPainterPathPx(
      const Length& expansion = Length(0)) const noexcept;

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setLayers(const Layer& from, const Layer& to);
  bool setPosition(const Point& position) noexcept;
  bool setSize(const PositiveLength& size) noexcept;
  bool setDrillDiameter(const PositiveLength& diameter) noexcept;
  bool setExposureConfig(const MaskConfig& config) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Via& rhs) const noexcept;
  bool operator!=(const Via& rhs) const noexcept { return !(*this == rhs); }
  Via& operator=(const Via& rhs) noexcept;

  // Static Methods
  static Path getOutline(const PositiveLength& size,
                         const Length& expansion = Length(0)) noexcept;
  static bool isOnLayer(const Layer& layer, const Layer& from,
                        const Layer& to) noexcept;
  static QPainterPath toQPainterPathPx(
      const PositiveLength& size, const PositiveLength& drillDiameter,
      const Length& expansion = Length(0)) noexcept;

private:  // Data
  Uuid mUuid;
  const Layer* mStartLayer;
  const Layer* mEndLayer;
  Point mPosition;
  PositiveLength mSize;
  PositiveLength mDrillDiameter;
  MaskConfig mExposureConfig;
};

/*******************************************************************************
 *  Class ViaList
 ******************************************************************************/

struct ViaListNameProvider {
  static constexpr const char* tagname = "via";
};
using ViaList = SerializableObjectList<Via, ViaListNameProvider, Via::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
