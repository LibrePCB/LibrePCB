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

#ifndef LIBREPCB_CORE_COMPONENTCHECKMESSAGES_H
#define LIBREPCB_CORE_COMPONENTCHECKMESSAGES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../rulecheck/rulecheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentSignal;
class ComponentSymbolVariant;

/*******************************************************************************
 *  Class MsgDuplicateSignalName
 ******************************************************************************/

/**
 * @brief The MsgDuplicateSignalName class
 */
class MsgDuplicateSignalName final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgDuplicateSignalName)

public:
  // Constructors / Destructor
  MsgDuplicateSignalName() = delete;
  explicit MsgDuplicateSignalName(const ComponentSignal& signal) noexcept;
  MsgDuplicateSignalName(const MsgDuplicateSignalName& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgDuplicateSignalName() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingComponentDefaultValue
 ******************************************************************************/

/**
 * @brief The MsgMissingComponentDefaultValue class
 */
class MsgMissingComponentDefaultValue final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingComponentDefaultValue)

public:
  // Constructors / Destructor
  MsgMissingComponentDefaultValue() noexcept;
  MsgMissingComponentDefaultValue(
      const MsgMissingComponentDefaultValue& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingComponentDefaultValue() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingComponentPrefix
 ******************************************************************************/

/**
 * @brief The MsgMissingComponentPrefix class
 */
class MsgMissingComponentPrefix final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingComponentPrefix)

public:
  // Constructors / Destructor
  MsgMissingComponentPrefix() noexcept;
  MsgMissingComponentPrefix(const MsgMissingComponentPrefix& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingComponentPrefix() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingSymbolVariant
 ******************************************************************************/

/**
 * @brief The MsgMissingSymbolVariant class
 */
class MsgMissingSymbolVariant final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingSymbolVariant)

public:
  // Constructors / Destructor
  MsgMissingSymbolVariant() noexcept;
  MsgMissingSymbolVariant(const MsgMissingSymbolVariant& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingSymbolVariant() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingSymbolVariantItem
 ******************************************************************************/

/**
 * @brief The MsgMissingSymbolVariantItem class
 */
class MsgMissingSymbolVariantItem final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingSymbolVariantItem)

public:
  // Constructors / Destructor
  MsgMissingSymbolVariantItem() noexcept;
  explicit MsgMissingSymbolVariantItem(
      std::shared_ptr<const ComponentSymbolVariant> symbVar) noexcept;
  MsgMissingSymbolVariantItem(const MsgMissingSymbolVariantItem& other) noexcept
    : RuleCheckMessage(other), mSymbVar(other.mSymbVar) {}
  virtual ~MsgMissingSymbolVariantItem() noexcept {}

  // Getters
  std::shared_ptr<const ComponentSymbolVariant> getSymbVar() const noexcept {
    return mSymbVar;
  }

private:
  std::shared_ptr<const ComponentSymbolVariant> mSymbVar;
};

/*******************************************************************************
 *  Class MsgNonFunctionalComponentSignalInversionSign
 ******************************************************************************/

/**
 * @brief The MsgNonFunctionalComponentSignalInversionSign class
 */
class MsgNonFunctionalComponentSignalInversionSign final
  : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgNonFunctionalComponentSignalInversionSign)

public:
  // Constructors / Destructor
  MsgNonFunctionalComponentSignalInversionSign() = delete;
  explicit MsgNonFunctionalComponentSignalInversionSign(
      std::shared_ptr<const ComponentSignal> signal) noexcept;
  MsgNonFunctionalComponentSignalInversionSign(
      const MsgNonFunctionalComponentSignalInversionSign& other) noexcept
    : RuleCheckMessage(other), mSignal(other.mSignal) {}
  virtual ~MsgNonFunctionalComponentSignalInversionSign() noexcept {}

  // Getters
  const std::shared_ptr<const ComponentSignal>& getSignal() const noexcept {
    return mSignal;
  }

private:
  std::shared_ptr<const ComponentSignal> mSignal;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
