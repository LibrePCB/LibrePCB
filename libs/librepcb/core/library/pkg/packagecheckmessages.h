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

#ifndef LIBREPCB_CORE_PACKAGECHECKMESSAGES_H
#define LIBREPCB_CORE_PACKAGECHECKMESSAGES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../rulecheck/rulecheckmessage.h"
#include "../../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Footprint;
class FootprintPad;
class Hole;
class PackagePad;
class StrokeText;

/*******************************************************************************
 *  Class MsgDeprecatedAssemblyType
 ******************************************************************************/

/**
 * @brief The MsgDeprecatedAssemblyType class
 */
class MsgDeprecatedAssemblyType final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgDeprecatedAssemblyType)

public:
  // Constructors / Destructor
  MsgDeprecatedAssemblyType() noexcept;
  MsgDeprecatedAssemblyType(const MsgDeprecatedAssemblyType& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgDeprecatedAssemblyType() noexcept {}
};

/*******************************************************************************
 *  Class MsgSuspiciousAssemblyType
 ******************************************************************************/

/**
 * @brief The MsgSuspiciousAssemblyType class
 */
class MsgSuspiciousAssemblyType final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgSuspiciousAssemblyType)

public:
  // Constructors / Destructor
  MsgSuspiciousAssemblyType() noexcept;
  MsgSuspiciousAssemblyType(const MsgSuspiciousAssemblyType& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgSuspiciousAssemblyType() noexcept {}
};

/*******************************************************************************
 *  Class MsgDuplicatePadName
 ******************************************************************************/

/**
 * @brief The MsgDuplicatePadName class
 */
class MsgDuplicatePadName final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgDuplicatePadName)

public:
  // Constructors / Destructor
  MsgDuplicatePadName() = delete;
  explicit MsgDuplicatePadName(const PackagePad& pad) noexcept;
  MsgDuplicatePadName(const MsgDuplicatePadName& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgDuplicatePadName() noexcept {}
};

/*******************************************************************************
 *  Class MsgHoleWithoutStopMask
 ******************************************************************************/

/**
 * @brief The MsgHoleWithoutStopMask class
 */
class MsgHoleWithoutStopMask final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgHoleWithoutStopMask)

public:
  // Constructors / Destructor
  MsgHoleWithoutStopMask() = delete;
  MsgHoleWithoutStopMask(std::shared_ptr<const Footprint> footprint,
                         std::shared_ptr<const Hole> hole) noexcept;
  MsgHoleWithoutStopMask(const MsgHoleWithoutStopMask& other) noexcept
    : RuleCheckMessage(other),
      mFootprint(other.mFootprint),
      mHole(other.mHole) {}
  virtual ~MsgHoleWithoutStopMask() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const Hole> getHole() const noexcept { return mHole; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const Hole> mHole;
};

/*******************************************************************************
 *  Class MsgInvalidCustomPadOutline
 ******************************************************************************/

/**
 * @brief The MsgInvalidCustomPadOutline class
 */
class MsgInvalidCustomPadOutline final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgInvalidCustomPadOutline)

public:
  // Constructors / Destructor
  MsgInvalidCustomPadOutline() = delete;
  MsgInvalidCustomPadOutline(std::shared_ptr<const Footprint> footprint,
                             std::shared_ptr<const FootprintPad> pad,
                             const QString& pkgPadName) noexcept;
  MsgInvalidCustomPadOutline(const MsgInvalidCustomPadOutline& other) noexcept
    : RuleCheckMessage(other), mFootprint(other.mFootprint), mPad(other.mPad) {}
  virtual ~MsgInvalidCustomPadOutline() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  Class MsgMissingFootprint
 ******************************************************************************/

/**
 * @brief The MsgMissingFootprint class
 */
class MsgMissingFootprint final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingFootprint)

public:
  // Constructors / Destructor
  MsgMissingFootprint() noexcept;
  MsgMissingFootprint(const MsgMissingFootprint& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingFootprint() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingFootprintName
 ******************************************************************************/

/**
 * @brief The MsgMissingFootprintName class
 */
class MsgMissingFootprintName final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingFootprintName)

public:
  // Constructors / Destructor
  MsgMissingFootprintName() = delete;
  explicit MsgMissingFootprintName(
      std::shared_ptr<const Footprint> footprint) noexcept;
  MsgMissingFootprintName(const MsgMissingFootprintName& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingFootprintName() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingFootprintValue
 ******************************************************************************/

/**
 * @brief The MsgMissingFootprintValue class
 */
class MsgMissingFootprintValue final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingFootprintValue)

public:
  // Constructors / Destructor
  MsgMissingFootprintValue() = delete;
  explicit MsgMissingFootprintValue(
      std::shared_ptr<const Footprint> footprint) noexcept;
  MsgMissingFootprintValue(const MsgMissingFootprintValue& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingFootprintValue() noexcept {}
};

/*******************************************************************************
 *  Class MsgOverlappingPads
 ******************************************************************************/

/**
 * @brief The MsgOverlappingPads class
 */
class MsgOverlappingPads final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgOverlappingPads)

public:
  // Constructors / Destructor
  MsgOverlappingPads() = delete;
  MsgOverlappingPads(std::shared_ptr<const Footprint> footprint,
                     std::shared_ptr<const FootprintPad> pad1,
                     const QString& pkgPad1Name,
                     std::shared_ptr<const FootprintPad> pad2,
                     const QString& pkgPad2Name) noexcept;
  MsgOverlappingPads(const MsgOverlappingPads& other) noexcept
    : RuleCheckMessage(other),
      mFootprint(other.mFootprint),
      mPad1(other.mPad1),
      mPad2(other.mPad2) {}
  virtual ~MsgOverlappingPads() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad1() const noexcept { return mPad1; }
  std::shared_ptr<const FootprintPad> getPad2() const noexcept { return mPad2; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad1;
  std::shared_ptr<const FootprintPad> mPad2;
};

/*******************************************************************************
 *  Class MsgPadAnnularRingViolation
 ******************************************************************************/

/**
 * @brief The MsgPadAnnularRingViolation class
 */
class MsgPadAnnularRingViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadAnnularRingViolation)

public:
  // Constructors / Destructor
  MsgPadAnnularRingViolation() = delete;
  MsgPadAnnularRingViolation(std::shared_ptr<const Footprint> footprint,
                             std::shared_ptr<const FootprintPad> pad,
                             const QString& pkgPadName,
                             const Length& annularRing) noexcept;
  MsgPadAnnularRingViolation(const MsgPadAnnularRingViolation& other) noexcept
    : RuleCheckMessage(other), mFootprint(other.mFootprint), mPad(other.mPad) {}
  virtual ~MsgPadAnnularRingViolation() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  Class MsgPadClearanceViolation
 ******************************************************************************/

/**
 * @brief The MsgPadClearanceViolation class
 */
class MsgPadClearanceViolation final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadClearanceViolation)

public:
  // Constructors / Destructor
  MsgPadClearanceViolation() = delete;
  MsgPadClearanceViolation(std::shared_ptr<const Footprint> footprint,
                           std::shared_ptr<const FootprintPad> pad1,
                           const QString& pkgPad1Name,
                           std::shared_ptr<const FootprintPad> pad2,
                           const QString& pkgPad2Name,
                           const Length& clearance) noexcept;
  MsgPadClearanceViolation(const MsgPadClearanceViolation& other) noexcept
    : RuleCheckMessage(other),
      mFootprint(other.mFootprint),
      mPad1(other.mPad1),
      mPad2(other.mPad2) {}
  virtual ~MsgPadClearanceViolation() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad1() const noexcept { return mPad1; }
  std::shared_ptr<const FootprintPad> getPad2() const noexcept { return mPad2; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad1;
  std::shared_ptr<const FootprintPad> mPad2;
};

/*******************************************************************************
 *  Class MsgPadHoleOutsideCopper
 ******************************************************************************/

/**
 * @brief The MsgPadHoleOutsideCopper class
 */
class MsgPadHoleOutsideCopper final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadHoleOutsideCopper)

public:
  // Constructors / Destructor
  MsgPadHoleOutsideCopper() = delete;
  MsgPadHoleOutsideCopper(std::shared_ptr<const Footprint> footprint,
                          std::shared_ptr<const FootprintPad> pad,
                          const QString& pkgPadName) noexcept;
  MsgPadHoleOutsideCopper(const MsgPadHoleOutsideCopper& other) noexcept
    : RuleCheckMessage(other), mFootprint(other.mFootprint), mPad(other.mPad) {}
  virtual ~MsgPadHoleOutsideCopper() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  Class MsgPadOriginOutsideCopper
 ******************************************************************************/

/**
 * @brief The MsgPadOriginOutsideCopper class
 */
class MsgPadOriginOutsideCopper final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadOriginOutsideCopper)

public:
  // Constructors / Destructor
  MsgPadOriginOutsideCopper() = delete;
  MsgPadOriginOutsideCopper(std::shared_ptr<const Footprint> footprint,
                            std::shared_ptr<const FootprintPad> pad,
                            const QString& pkgPadName) noexcept;
  MsgPadOriginOutsideCopper(const MsgPadOriginOutsideCopper& other) noexcept
    : RuleCheckMessage(other), mFootprint(other.mFootprint), mPad(other.mPad) {}
  virtual ~MsgPadOriginOutsideCopper() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  Class MsgPadOverlapsWithPlacement
 ******************************************************************************/

/**
 * @brief The MsgPadOverlapsWithPlacement class
 */
class MsgPadOverlapsWithPlacement final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgPadOverlapsWithPlacement)

public:
  // Constructors / Destructor
  MsgPadOverlapsWithPlacement() = delete;
  MsgPadOverlapsWithPlacement(std::shared_ptr<const Footprint> footprint,
                              std::shared_ptr<const FootprintPad> pad,
                              const QString& pkgPadName,
                              const Length& clearance) noexcept;
  MsgPadOverlapsWithPlacement(const MsgPadOverlapsWithPlacement& other) noexcept
    : RuleCheckMessage(other), mFootprint(other.mFootprint), mPad(other.mPad) {}
  virtual ~MsgPadOverlapsWithPlacement() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  Class MsgUnusedCustomPadOutline
 ******************************************************************************/

/**
 * @brief The MsgUnusedCustomPadOutline class
 */
class MsgUnusedCustomPadOutline final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgUnusedCustomPadOutline)

public:
  // Constructors / Destructor
  MsgUnusedCustomPadOutline() = delete;
  MsgUnusedCustomPadOutline(std::shared_ptr<const Footprint> footprint,
                            std::shared_ptr<const FootprintPad> pad,
                            const QString& pkgPadName) noexcept;
  MsgUnusedCustomPadOutline(const MsgUnusedCustomPadOutline& other) noexcept
    : RuleCheckMessage(other), mFootprint(other.mFootprint), mPad(other.mPad) {}
  virtual ~MsgUnusedCustomPadOutline() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const FootprintPad> getPad() const noexcept { return mPad; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const FootprintPad> mPad;
};

/*******************************************************************************
 *  Class MsgWrongFootprintTextLayer
 ******************************************************************************/

/**
 * @brief The MsgWrongFootprintTextLayer class
 */
class MsgWrongFootprintTextLayer final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgWrongFootprintTextLayer)

public:
  // Constructors / Destructor
  MsgWrongFootprintTextLayer() = delete;
  MsgWrongFootprintTextLayer(std::shared_ptr<const Footprint> footprint,
                             std::shared_ptr<const StrokeText> text,
                             const QString& expectedLayerName) noexcept;
  MsgWrongFootprintTextLayer(const MsgWrongFootprintTextLayer& other) noexcept
    : RuleCheckMessage(other),
      mFootprint(other.mFootprint),
      mText(other.mText),
      mExpectedLayerName(other.mExpectedLayerName) {}
  virtual ~MsgWrongFootprintTextLayer() noexcept {}

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const StrokeText> getText() const noexcept { return mText; }
  QString getExpectedLayerName() const noexcept { return mExpectedLayerName; }

private:
  std::shared_ptr<const Footprint> mFootprint;
  std::shared_ptr<const StrokeText> mText;
  QString mExpectedLayerName;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
