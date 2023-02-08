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

#ifndef LIBREPCB_CORE_BOARDDESIGNRULES_H
#define LIBREPCB_CORE_BOARDDESIGNRULES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/length.h"
#include "../../types/ratio.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/*******************************************************************************
 *  Class BoardDesignRules
 ******************************************************************************/

/**
 * @brief The BoardDesignRules class
 */
class BoardDesignRules final {
  Q_DECLARE_TR_FUNCTIONS(BoardDesignRules)

public:
  // Constructors / Destructor
  BoardDesignRules() noexcept;
  BoardDesignRules(const BoardDesignRules& other);
  explicit BoardDesignRules(const SExpression& node);
  ~BoardDesignRules() noexcept;

  // Getters: Stop Mask
  const UnsignedLength& getStopMaskMaxViaDiameter() const noexcept {
    return mStopMaskMaxViaDrillDiameter;
  }
  const UnsignedRatio& getStopMaskClearanceRatio() const noexcept {
    return mStopMaskClearanceRatio;
  }
  const UnsignedLength& getStopMaskClearanceMin() const noexcept {
    return mStopMaskClearanceMin;
  }
  const UnsignedLength& getStopMaskClearanceMax() const noexcept {
    return mStopMaskClearanceMax;
  }

  // Getters: Solder Paste
  const UnsignedRatio& getSolderPasteClearanceRatio() const noexcept {
    return mSolderPasteClearanceRatio;
  }
  const UnsignedLength& getSolderPasteClearanceMin() const noexcept {
    return mSolderPasteClearanceMin;
  }
  const UnsignedLength& getSolderPasteClearanceMax() const noexcept {
    return mSolderPasteClearanceMax;
  }

  // Getters: Pad Annular Ring
  bool getPadCmpSideAutoAnnularRing() const noexcept {
    return mPadCmpSideAutoAnnularRing;
  }
  bool getPadInnerAutoAnnularRing() const noexcept {
    return mPadInnerAutoAnnularRing;
  }
  const UnsignedRatio& getPadAnnularRingRatio() const noexcept {
    return mPadAnnularRingRatio;
  }
  const UnsignedLength& getPadAnnularRingMin() const noexcept {
    return mPadAnnularRingMin;
  }
  const UnsignedLength& getPadAnnularRingMax() const noexcept {
    return mPadAnnularRingMax;
  }

  // Getters: Via Annular Ring
  const UnsignedRatio& getViaAnnularRingRatio() const noexcept {
    return mViaAnnularRingRatio;
  }
  const UnsignedLength& getViaAnnularRingMin() const noexcept {
    return mViaAnnularRingMin;
  }
  const UnsignedLength& getViaAnnularRingMax() const noexcept {
    return mViaAnnularRingMax;
  }

  // Setters
  void setStopMaskMaxViaDiameter(const UnsignedLength& dia) noexcept {
    mStopMaskMaxViaDrillDiameter = dia;
  }
  void setStopMaskClearance(const UnsignedRatio& ratio,
                            const UnsignedLength& min,
                            const UnsignedLength& max);
  void setSolderPasteClearance(const UnsignedRatio& ratio,
                               const UnsignedLength& min,
                               const UnsignedLength& max);
  void setPadCmpSideAutoAnnularRing(bool enabled) noexcept;
  void setPadInnerAutoAnnularRing(bool enabled) noexcept;
  void setPadAnnularRing(const UnsignedRatio& ratio, const UnsignedLength& min,
                         const UnsignedLength& max);
  void setViaAnnularRing(const UnsignedRatio& ratio, const UnsignedLength& min,
                         const UnsignedLength& max);

  // General Methods
  void restoreDefaults() noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Helper Methods
  bool doesViaRequireStopMask(const Length& drillDia) const noexcept;
  UnsignedLength calcStopMaskClearance(const Length& padSize) const noexcept;
  UnsignedLength calcSolderPasteClearance(const Length& padSize) const noexcept;
  UnsignedLength calcPadAnnularRing(const Length& drillDia) const noexcept;
  UnsignedLength calcViaAnnularRing(const Length& drillDia) const noexcept;

  // Operator Overloadings
  BoardDesignRules& operator=(const BoardDesignRules& rhs) noexcept;

private:  // Methods
  static bool parsePadAutoAnnular(const SExpression& node);

private:  // Data
  // Stop Mask
  UnsignedLength mStopMaskMaxViaDrillDiameter;
  UnsignedRatio mStopMaskClearanceRatio;
  UnsignedLength mStopMaskClearanceMin;
  UnsignedLength mStopMaskClearanceMax;

  // Solder Paste
  UnsignedRatio mSolderPasteClearanceRatio;
  UnsignedLength mSolderPasteClearanceMin;
  UnsignedLength mSolderPasteClearanceMax;

  // Pad Annular Ring
  bool mPadCmpSideAutoAnnularRing;
  bool mPadInnerAutoAnnularRing;
  UnsignedRatio mPadAnnularRingRatio;  /// Percentage of the drill diameter
  UnsignedLength mPadAnnularRingMin;
  UnsignedLength mPadAnnularRingMax;

  // Via Annular Ring
  UnsignedRatio mViaAnnularRingRatio;  /// Percentage of the drill diameter
  UnsignedLength mViaAnnularRingMin;
  UnsignedLength mViaAnnularRingMax;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
