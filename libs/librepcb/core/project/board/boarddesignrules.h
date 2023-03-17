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
#include "../../types/boundedunsignedratio.h"
#include "../../types/length.h"

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
  const BoundedUnsignedRatio& getStopMaskClearance() const noexcept {
    return mStopMaskClearance;
  }

  // Getters: Solder Paste
  const BoundedUnsignedRatio& getSolderPasteClearance() const noexcept {
    return mSolderPasteClearance;
  }

  // Getters: Pad Annular Ring
  bool getPadCmpSideAutoAnnularRing() const noexcept {
    return mPadCmpSideAutoAnnularRing;
  }
  bool getPadInnerAutoAnnularRing() const noexcept {
    return mPadInnerAutoAnnularRing;
  }
  const BoundedUnsignedRatio& getPadAnnularRing() const noexcept {
    return mPadAnnularRing;
  }

  // Getters: Via Annular Ring
  const BoundedUnsignedRatio& getViaAnnularRing() const noexcept {
    return mViaAnnularRing;
  }

  // Setters
  void setStopMaskMaxViaDiameter(const UnsignedLength& dia) noexcept {
    mStopMaskMaxViaDrillDiameter = dia;
  }
  void setStopMaskClearance(const BoundedUnsignedRatio& value) noexcept {
    mStopMaskClearance = value;
  }
  void setSolderPasteClearance(const BoundedUnsignedRatio& value) noexcept {
    mSolderPasteClearance = value;
  }
  void setPadCmpSideAutoAnnularRing(bool enabled) noexcept {
    mPadCmpSideAutoAnnularRing = enabled;
  }
  void setPadInnerAutoAnnularRing(bool enabled) noexcept {
    mPadInnerAutoAnnularRing = enabled;
  }
  void setPadAnnularRing(const BoundedUnsignedRatio& value) {
    mPadAnnularRing = value;
  }
  void setViaAnnularRing(const BoundedUnsignedRatio& value) {
    mViaAnnularRing = value;
  }

  // General Methods
  void restoreDefaults() noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Helper Methods
  bool doesViaRequireStopMaskOpening(const Length& drillDia) const noexcept;

  // Operator Overloadings
  BoardDesignRules& operator=(const BoardDesignRules& rhs) noexcept;
  bool operator==(const BoardDesignRules& rhs) const noexcept;
  bool operator!=(const BoardDesignRules& rhs) const noexcept {
    return !(*this == rhs);
  }

private:  // Methods
  static bool parsePadAutoAnnular(const SExpression& node);

private:  // Data
  // Stop Mask
  UnsignedLength mStopMaskMaxViaDrillDiameter;
  BoundedUnsignedRatio mStopMaskClearance;

  // Solder Paste
  BoundedUnsignedRatio mSolderPasteClearance;

  // Pad Annular Ring
  bool mPadCmpSideAutoAnnularRing;
  bool mPadInnerAutoAnnularRing;
  BoundedUnsignedRatio mPadAnnularRing;  /// Percentage of the drill diameter

  // Via Annular Ring
  BoundedUnsignedRatio mViaAnnularRing;  /// Percentage of the drill diameter
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
