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

#ifndef LIBREPCB_CORE_PCBMANUFACTURERCAPABILITIES_H
#define LIBREPCB_CORE_PCBMANUFACTURERCAPABILITIES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializablekeyvaluemap.h"
#include "../../types/length.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PcbColor;
class SExpression;

/*******************************************************************************
 *  Class PcbManufacturerCapabilities
 ******************************************************************************/

/**
 * @brief The PcbManufacturerCapabilities class
 */
class PcbManufacturerCapabilities final {
public:
  // Types
  enum class AllowedSlots : int {
    None = 0,  ///< No slots are allowed at all.
    SingleSegmentStraight = 1,  ///< Straight single-segment slots are allowed.
    MultiSegmentStraight = 2,  ///< Straight multi-segment slots are allowed.
    Any = 3,  ///< Any kind of slot is allowed (including curves).
  };

  // Constructors / Destructor
  PcbManufacturerCapabilities() noexcept = delete;
  PcbManufacturerCapabilities(
      const PcbManufacturerCapabilities& other) noexcept;
  explicit PcbManufacturerCapabilities(const SExpression& node);
  ~PcbManufacturerCapabilities() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const LocalizedNameMap& getNames() const noexcept { return mNames; }
  const LocalizedDescriptionMap& getDescriptions() const noexcept {
    return mDescriptions;
  }
  const QUrl& getUrl() const noexcept { return mUrl; }
  const std::pair<UnsignedLength, UnsignedLength>& getMinBoardSize()
      const noexcept {
    return mMinBoardSize;
  }
  const std::pair<UnsignedLength, UnsignedLength>& getMaxBoardSizeDoubleSided()
      const noexcept {
    return mMaxBoardSizeDoubleSided;
  }
  const std::pair<UnsignedLength, UnsignedLength>& getMaxBoardSizeMultiLayer()
      const noexcept {
    return mMaxBoardSizeMultiLayer;
  }
  const QSet<PositiveLength>& getPcbThickness() const noexcept {
    return mPcbThickness;
  }
  uint getMaxInnerLayerCount() const noexcept { return mMaxInnerLayerCount; }
  const QSet<const PcbColor*>& getSolderResist() const noexcept {
    return mSolderResist;
  }
  const QSet<const PcbColor*>& getSilkscreen() const noexcept {
    return mSilkscreen;
  }
  const UnsignedLength& getMinCopperCopperClearance() const noexcept {
    return mMinCopperCopperClearance;
  }
  const UnsignedLength& getMinCopperBoardClearance() const noexcept {
    return mMinCopperBoardClearance;
  }
  const UnsignedLength& getMinCopperNpthClearance() const noexcept {
    return mMinCopperNpthClearance;
  }
  const UnsignedLength& getMinDrillDrillClearance() const noexcept {
    return mMinDrillDrillClearance;
  }
  const UnsignedLength& getMinDrillBoardClearance() const noexcept {
    return mMinDrillBoardClearance;
  }
  const UnsignedLength& getMinSilkscreenStopmaskClearance() const noexcept {
    return mMinSilkscreenStopmaskClearance;
  }
  const UnsignedLength& getMinCopperWidth() const noexcept {
    return mMinCopperWidth;
  }
  const UnsignedLength& getMinPthAnnularRing() const noexcept {
    return mMinPthAnnularRing;
  }
  const UnsignedLength& getMinNpthDrillDiameter() const noexcept {
    return mMinNpthDrillDiameter;
  }
  const UnsignedLength& getMinPthDrillDiameter() const noexcept {
    return mMinPthDrillDiameter;
  }
  const UnsignedLength& getMinNpthSlotWidth() const noexcept {
    return mMinNpthSlotWidth;
  }
  const UnsignedLength& getMinPthSlotWidth() const noexcept {
    return mMinPthSlotWidth;
  }
  const UnsignedLength& getMinSilkscreenWidth() const noexcept {
    return mMinSilkscreenWidth;
  }
  const UnsignedLength& getMinSilkscreenTextHeight() const noexcept {
    return mMinSilkscreenTextHeight;
  }
  const UnsignedLength& getMinOutlineToolDiameter() const noexcept {
    return mMinOutlineToolDiameter;
  }
  bool getBlindViasAllowed() const noexcept { return mBlindViasAllowed; }
  bool getBuriedViasAllowed() const noexcept { return mBuriedViasAllowed; }
  AllowedSlots getAllowedNpthSlots() const noexcept {
    return mAllowedNpthSlots;
  }
  AllowedSlots getAllowedPthSlots() const noexcept { return mAllowedPthSlots; }

  // Setters
  void setNames(const LocalizedNameMap& names) noexcept { mNames = names; }
  void setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept {
    mDescriptions = descriptions;
  }
  void setMinCopperCopperClearance(const UnsignedLength& value) noexcept {
    mMinCopperCopperClearance = value;
  }
  void setMinCopperBoardClearance(const UnsignedLength& value) noexcept {
    mMinCopperBoardClearance = value;
  }
  void setMinCopperNpthClearance(const UnsignedLength& value) noexcept {
    mMinCopperNpthClearance = value;
  }
  void setMinDrillDrillClearance(const UnsignedLength& value) noexcept {
    mMinDrillDrillClearance = value;
  }
  void setMinDrillBoardClearance(const UnsignedLength& value) noexcept {
    mMinDrillBoardClearance = value;
  }
  void setMinSilkscreenStopmaskClearance(const UnsignedLength& value) noexcept {
    mMinSilkscreenStopmaskClearance = value;
  }
  void setMinCopperWidth(const UnsignedLength& value) noexcept {
    mMinCopperWidth = value;
  }
  void setMinPthAnnularRing(const UnsignedLength& value) noexcept {
    mMinPthAnnularRing = value;
  }
  void setMinNpthDrillDiameter(const UnsignedLength& value) noexcept {
    mMinNpthDrillDiameter = value;
  }
  void setMinPthDrillDiameter(const UnsignedLength& value) noexcept {
    mMinPthDrillDiameter = value;
  }
  void setMinNpthSlotWidth(const UnsignedLength& value) noexcept {
    mMinNpthSlotWidth = value;
  }
  void setMinPthSlotWidth(const UnsignedLength& value) noexcept {
    mMinPthSlotWidth = value;
  }
  void setMinSilkscreenWidth(const UnsignedLength& value) noexcept {
    mMinSilkscreenWidth = value;
  }
  void setMinSilkscreenTextHeight(const UnsignedLength& value) noexcept {
    mMinSilkscreenTextHeight = value;
  }
  void setMinOutlineToolDiameter(const UnsignedLength& value) noexcept {
    mMinOutlineToolDiameter = value;
  }
  void setBlindViasAllowed(bool value) noexcept { mBlindViasAllowed = value; }
  void setBuriedViasAllowed(bool value) noexcept { mBuriedViasAllowed = value; }
  void setAllowedNpthSlots(AllowedSlots value) noexcept {
    mAllowedNpthSlots = value;
  }
  void setAllowedPthSlots(AllowedSlots value) noexcept {
    mAllowedPthSlots = value;
  }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  PcbManufacturerCapabilities& operator=(
      const PcbManufacturerCapabilities& rhs) noexcept;

private:  // Data
  // Attributes
  Uuid mUuid;
  LocalizedNameMap mNames;
  LocalizedDescriptionMap mDescriptions;
  QUrl mUrl;

  // General
  std::pair<UnsignedLength, UnsignedLength> mMinBoardSize;
  std::pair<UnsignedLength, UnsignedLength> mMaxBoardSizeDoubleSided;
  std::pair<UnsignedLength, UnsignedLength> mMaxBoardSizeMultiLayer;
  QSet<PositiveLength> mPcbThickness;
  uint mMaxInnerLayerCount;
  QSet<const PcbColor*> mSolderResist;  ///< May include `nullptr`
  QSet<const PcbColor*> mSilkscreen;  ///< May include `nullptr`

  // Clearances
  UnsignedLength mMinCopperCopperClearance;
  UnsignedLength mMinCopperBoardClearance;
  UnsignedLength mMinCopperNpthClearance;
  UnsignedLength mMinDrillDrillClearance;
  UnsignedLength mMinDrillBoardClearance;
  UnsignedLength mMinSilkscreenStopmaskClearance;

  // Minimum sizes
  UnsignedLength mMinCopperWidth;
  UnsignedLength mMinPthAnnularRing;
  UnsignedLength mMinNpthDrillDiameter;
  UnsignedLength mMinPthDrillDiameter;
  UnsignedLength mMinNpthSlotWidth;
  UnsignedLength mMinPthSlotWidth;
  UnsignedLength mMinSilkscreenWidth;
  UnsignedLength mMinSilkscreenTextHeight;
  UnsignedLength mMinOutlineToolDiameter;

  // Allowed features
  bool mBlindViasAllowed;
  bool mBuriedViasAllowed;
  AllowedSlots mAllowedNpthSlots;
  AllowedSlots mAllowedPthSlots;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::PcbManufacturerCapabilities::AllowedSlots)

#endif
