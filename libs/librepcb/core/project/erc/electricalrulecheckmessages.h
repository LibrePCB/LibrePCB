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

#ifndef LIBREPCB_CORE_ELECTRICALRULECHECKMESSAGES_H
#define LIBREPCB_CORE_ELECTRICALRULECHECKMESSAGES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../rulecheck/rulecheckmessage.h"
#include "../../types/uuid.h"

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class ComponentSignalInstance;
class ComponentSymbolVariantItem;
class NetClass;
class NetSignal;
class SI_NetLine;
class SI_NetPoint;
class SI_NetSegment;
class SI_Symbol;
class SI_SymbolPin;

/*******************************************************************************
 *  Class ErcMsgBase
 ******************************************************************************/

/**
 * @brief Base class for all ERC messages
 *
 * Provides the functionality for the "go to problem" feature for all ERC
 * messages. Constructors of derived classes can just call one of the
 * #setLocation() methods to specify the location of the problem.
 */
class ErcMsgBase : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgBase)

public:
  // Constructors / Destructor
  ErcMsgBase() = delete;
  explicit ErcMsgBase(Severity severity, const QString& msg,
                      const QString& description,
                      const QString& approvalName) noexcept
    : RuleCheckMessage(severity, msg, description, approvalName),
      mSchematic() {}
  ErcMsgBase(const ErcMsgBase& other) noexcept
    : RuleCheckMessage(other), mSchematic(other.mSchematic) {}
  virtual ~ErcMsgBase() noexcept {}

  // Getters
  const std::optional<Uuid>& getSchematic() const noexcept {
    return mSchematic;
  }

protected:
  bool setLocation(const NetSignal& net) noexcept;
  bool setLocation(const ComponentInstance& component) noexcept;
  bool setLocation(const ComponentSignalInstance& signal) noexcept;
  void setLocation(const SI_Symbol& symbol) noexcept;
  void setLocation(const SI_SymbolPin& pin) noexcept;
  bool setLocation(const SI_NetSegment& segment) noexcept;
  void setLocation(const SI_NetPoint& netPoint) noexcept;
  void setLocation(const SI_NetLine& netLine) noexcept;

  std::optional<Uuid> mSchematic;
};

/*******************************************************************************
 *  Class ErcMsgUnusedNetClass
 ******************************************************************************/

/**
 * @brief The ErcMsgUnusedNetClass class
 */
class ErcMsgUnusedNetClass final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgUnusedNetClass)

public:
  // Constructors / Destructor
  ErcMsgUnusedNetClass() = delete;
  explicit ErcMsgUnusedNetClass(const NetClass& netClass) noexcept;
  ErcMsgUnusedNetClass(const ErcMsgUnusedNetClass& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgUnusedNetClass() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgOpenNet
 ******************************************************************************/

/**
 * @brief The ErcMsgOpenNet class
 */
class ErcMsgOpenNet final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgOpenNet)

public:
  // Constructors / Destructor
  ErcMsgOpenNet() = delete;
  explicit ErcMsgOpenNet(const NetSignal& net) noexcept;
  ErcMsgOpenNet(const ErcMsgOpenNet& other) noexcept : ErcMsgBase(other) {}
  virtual ~ErcMsgOpenNet() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgOpenWireInSegment
 ******************************************************************************/

/**
 * @brief The ErcMsgOpenWireInSegment class
 */
class ErcMsgOpenWireInSegment final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgOpenWireInSegment)

public:
  // Constructors / Destructor
  ErcMsgOpenWireInSegment() = delete;
  explicit ErcMsgOpenWireInSegment(const SI_NetSegment& segment,
                                   const SI_NetLine& openWire) noexcept;
  ErcMsgOpenWireInSegment(const ErcMsgOpenWireInSegment& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgOpenWireInSegment() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgUnconnectedRequiredSignal
 ******************************************************************************/

/**
 * @brief The ErcMsgUnconnectedRequiredSignal class
 */
class ErcMsgUnconnectedRequiredSignal final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgUnconnectedRequiredSignal)

public:
  // Constructors / Destructor
  ErcMsgUnconnectedRequiredSignal() = delete;
  explicit ErcMsgUnconnectedRequiredSignal(
      const ComponentSignalInstance& signal) noexcept;
  ErcMsgUnconnectedRequiredSignal(
      const ErcMsgUnconnectedRequiredSignal& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgUnconnectedRequiredSignal() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgForcedNetSignalNameConflict
 ******************************************************************************/

/**
 * @brief The ErcMsgForcedNetSignalNameConflict class
 */
class ErcMsgForcedNetSignalNameConflict final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgForcedNetSignalNameConflict)

public:
  // Constructors / Destructor
  ErcMsgForcedNetSignalNameConflict() = delete;
  explicit ErcMsgForcedNetSignalNameConflict(
      const ComponentSignalInstance& signal) noexcept;
  ErcMsgForcedNetSignalNameConflict(
      const ErcMsgForcedNetSignalNameConflict& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgForcedNetSignalNameConflict() noexcept {}

private:
  static QString getSignalNet(const ComponentSignalInstance& signal) noexcept;
};

/*******************************************************************************
 *  Class ErcMsgUnplacedRequiredGate
 ******************************************************************************/

/**
 * @brief The ErcMsgUnplacedRequiredGate class
 */
class ErcMsgUnplacedRequiredGate final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgUnplacedRequiredSymbol)

public:
  // Constructors / Destructor
  ErcMsgUnplacedRequiredGate() = delete;
  explicit ErcMsgUnplacedRequiredGate(
      const ComponentInstance& component,
      const ComponentSymbolVariantItem& gate) noexcept;
  ErcMsgUnplacedRequiredGate(const ErcMsgUnplacedRequiredGate& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgUnplacedRequiredGate() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgUnplacedOptionalGate
 ******************************************************************************/

/**
 * @brief The ErcMsgUnplacedOptionalGate class
 */
class ErcMsgUnplacedOptionalGate final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgUnplacedOptionalSymbol)

public:
  // Constructors / Destructor
  ErcMsgUnplacedOptionalGate() = delete;
  explicit ErcMsgUnplacedOptionalGate(
      const ComponentInstance& component,
      const ComponentSymbolVariantItem& gate) noexcept;
  ErcMsgUnplacedOptionalGate(const ErcMsgUnplacedOptionalGate& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgUnplacedOptionalGate() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgConnectedPinWithoutWire
 ******************************************************************************/

/**
 * @brief The ErcMsgConnectedPinWithoutWire class
 */
class ErcMsgConnectedPinWithoutWire final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgConnectedPinWithoutWire)

public:
  // Constructors / Destructor
  ErcMsgConnectedPinWithoutWire() = delete;
  explicit ErcMsgConnectedPinWithoutWire(const SI_SymbolPin& pin) noexcept;
  ErcMsgConnectedPinWithoutWire(
      const ErcMsgConnectedPinWithoutWire& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgConnectedPinWithoutWire() noexcept {}
};

/*******************************************************************************
 *  Class ErcMsgUnconnectedJunction
 ******************************************************************************/

/**
 * @brief The ErcMsgUnconnectedJunction class
 */
class ErcMsgUnconnectedJunction final : public ErcMsgBase {
  Q_DECLARE_TR_FUNCTIONS(ErcMsgUnconnectedJunction)

public:
  // Constructors / Destructor
  ErcMsgUnconnectedJunction() = delete;
  explicit ErcMsgUnconnectedJunction(const SI_NetPoint& netPoint) noexcept;
  ErcMsgUnconnectedJunction(const ErcMsgUnconnectedJunction& other) noexcept
    : ErcMsgBase(other) {}
  virtual ~ErcMsgUnconnectedJunction() noexcept {}
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
