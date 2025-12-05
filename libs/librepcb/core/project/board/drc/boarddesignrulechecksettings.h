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

#ifndef LIBREPCB_CORE_BOARDDESIGNRULECHECKSETTINGS_H
#define LIBREPCB_CORE_BOARDDESIGNRULECHECKSETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../types/elementname.h"
#include "../../../types/length.h"
#include "../../../types/uuid.h"
#include "../../../types/version.h"

#include <unordered_set>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PcbColor;
class SExpression;

/*******************************************************************************
 *  Class BoardDesignRuleCheckSettings
 ******************************************************************************/

/**
 * @brief The BoardDesignRuleCheckSettings class
 */
class BoardDesignRuleCheckSettings final {
public:
  // Types
  struct Source {
    Uuid organizationUuid;
    ElementName organizationName;
    Version organizationVersion;
    Uuid pcbDesignRulesUuid;
    ElementName pcbDesignRulesName;

    static Source load(const SExpression& node);
    void serialize(SExpression& root) const;
    bool operator==(const Source& rhs) const noexcept = default;
  };
  struct SourceSetCmp {
    std::size_t operator()(const Source& s) const noexcept {
      return ::qHash(std::make_pair(s.organizationUuid, s.pcbDesignRulesUuid));
    }
    bool operator()(const Source& lhs, const Source& rhs) const noexcept {
      return (lhs.organizationUuid == rhs.organizationUuid) &&
          (lhs.pcbDesignRulesUuid == rhs.pcbDesignRulesUuid);
    }
  };
  typedef std::unordered_set<Source, SourceSetCmp, SourceSetCmp> SourceSet;
  enum class AllowedSlots : int {
    None = 0,  ///< No slots are allowed at all.
    SingleSegmentStraight = 1,  ///< Straight single-segment slots are allowed.
    MultiSegmentStraight = 2,  ///< Straight multi-segment slots are allowed.
    Any = 3,  ///< Any kind of slot is allowed (including curves).
  };

  // Constructors / Destructor
  BoardDesignRuleCheckSettings() noexcept;
  BoardDesignRuleCheckSettings(
      const BoardDesignRuleCheckSettings& other) noexcept;
  explicit BoardDesignRuleCheckSettings(const SExpression& node);
  ~BoardDesignRuleCheckSettings() noexcept;

  // Getters
  const SourceSet& getSources() const noexcept { return mSources; }
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
  uint getMaxLayerCount() const noexcept { return mMaxLayerCount; }
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
  const UnsignedLength& getMaxTentedViaDrillDiameter() const noexcept {
    return mMaxTentedViaDrillDiameter;
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
  const QMap<QString, QList<SExpression>>& getOptions() const noexcept {
    return mOptions;
  }

  // Setters
  void setSources(const SourceSet& value) noexcept { mSources = value; }
  void setMinBoardSize(
      const std::pair<UnsignedLength, UnsignedLength>& value) noexcept {
    mMinBoardSize = value;
  }
  void setMaxBoardSizeDoubleSided(
      const std::pair<UnsignedLength, UnsignedLength>& value) noexcept {
    mMaxBoardSizeDoubleSided = value;
  }
  void setMaxBoardSizeMultiLayer(
      const std::pair<UnsignedLength, UnsignedLength>& value) noexcept {
    mMaxBoardSizeMultiLayer = value;
  }
  void setPcbThickness(const QSet<PositiveLength>& value) noexcept {
    mPcbThickness = value;
  }
  void setMaxLayerCount(uint value) noexcept { mMaxLayerCount = value; }
  void setSolderResist(const QSet<const PcbColor*>& value) noexcept {
    mSolderResist = value;
  }
  void setSilkscreen(const QSet<const PcbColor*>& value) noexcept {
    mSilkscreen = value;
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
  void setMaxTentedViaDrillDiameter(const UnsignedLength& value) noexcept {
    mMaxTentedViaDrillDiameter = value;
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
  void setOptions(const QMap<QString, QList<SExpression>>& value) noexcept {
    mOptions = value;
  }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  BoardDesignRuleCheckSettings& operator=(
      const BoardDesignRuleCheckSettings& rhs) noexcept;
  bool operator==(const BoardDesignRuleCheckSettings& rhs) const noexcept;
  bool operator!=(const BoardDesignRuleCheckSettings& rhs) const noexcept {
    return !(*this == rhs);
  }

private:  // Data
  // Internal data
  SourceSet mSources;

  // General PCB manufacturer capabilities
  std::pair<UnsignedLength, UnsignedLength> mMinBoardSize;
  std::pair<UnsignedLength, UnsignedLength> mMaxBoardSizeDoubleSided;
  std::pair<UnsignedLength, UnsignedLength> mMaxBoardSizeMultiLayer;
  QSet<PositiveLength> mPcbThickness;  ///< No restrictions if empty
  uint mMaxLayerCount;  ///< 0 = Check disabled
  QSet<const PcbColor*> mSolderResist;  ///< Empty=any/unknown. `nullptr`=none
  QSet<const PcbColor*> mSilkscreen;  ///< Empty=any/unknown. `nullptr`=none

  // Clearances
  UnsignedLength mMinCopperCopperClearance;
  UnsignedLength mMinCopperBoardClearance;
  UnsignedLength mMinCopperNpthClearance;
  UnsignedLength mMinDrillDrillClearance;
  UnsignedLength mMinDrillBoardClearance;
  UnsignedLength mMinSilkscreenStopmaskClearance;

  // Minimum/maximum sizes
  UnsignedLength mMinCopperWidth;
  UnsignedLength mMinPthAnnularRing;
  UnsignedLength mMinNpthDrillDiameter;
  UnsignedLength mMinPthDrillDiameter;
  UnsignedLength mMinNpthSlotWidth;
  UnsignedLength mMinPthSlotWidth;
  UnsignedLength mMaxTentedViaDrillDiameter;
  UnsignedLength mMinSilkscreenWidth;
  UnsignedLength mMinSilkscreenTextHeight;
  UnsignedLength mMinOutlineToolDiameter;

  // Allowed features
  bool mBlindViasAllowed;
  bool mBuriedViasAllowed;
  AllowedSlots mAllowedNpthSlots;
  AllowedSlots mAllowedPthSlots;

  // Arbitrary options for forward compatibility in case we really need to
  // add new settings in a minor release.
  QMap<QString, QList<SExpression>> mOptions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::BoardDesignRuleCheckSettings::AllowedSlots)

#endif
