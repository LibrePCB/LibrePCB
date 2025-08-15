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

#ifndef LIBREPCB_CORE_SYMBOLCHECKMESSAGES_H
#define LIBREPCB_CORE_SYMBOLCHECKMESSAGES_H

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

class Layer;
class SymbolPin;
class Text;

/*******************************************************************************
 *  Class MsgDuplicatePinName
 ******************************************************************************/

/**
 * @brief The MsgDuplicatePinName class
 */
class MsgDuplicatePinName final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgDuplicatePinName)

public:
  // Constructors / Destructor
  MsgDuplicatePinName() = delete;
  explicit MsgDuplicatePinName(const SymbolPin& pin) noexcept;
  MsgDuplicatePinName(const MsgDuplicatePinName& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgDuplicatePinName() noexcept {}
};

/*******************************************************************************
 *  Class MsgInvalidImageFile
 ******************************************************************************/

/**
 * @brief The MsgInvalidImageFile class
 */
class MsgInvalidImageFile final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgInvalidImageFile)

public:
  enum class Error {
    FileMissing,
    FileReadError,
    UnsupportedFormat,
    ImageLoadError,
  };

  // Constructors / Destructor
  MsgInvalidImageFile() = delete;
  explicit MsgInvalidImageFile(const QString& fileName, Error error,
                               const QString& details) noexcept;
  MsgInvalidImageFile(const MsgInvalidImageFile& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgInvalidImageFile() noexcept {}

private:
  static QString buildMessagePattern(Error error) noexcept;
  static QString buildDescription(const QString& details) noexcept;
};

/*******************************************************************************
 *  Class MsgMissingSymbolName
 ******************************************************************************/

/**
 * @brief The MsgMissingSymbolName class
 */
class MsgMissingSymbolName final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingSymbolName)

public:
  // Constructors / Destructor
  MsgMissingSymbolName() noexcept;
  MsgMissingSymbolName(const MsgMissingSymbolName& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingSymbolName() noexcept {}
};

/*******************************************************************************
 *  Class MsgMissingSymbolValue
 ******************************************************************************/

/**
 * @brief The MsgMissingSymbolValue class
 */
class MsgMissingSymbolValue final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgMissingSymbolValue)

public:
  // Constructors / Destructor
  MsgMissingSymbolValue() noexcept;
  MsgMissingSymbolValue(const MsgMissingSymbolValue& other) noexcept
    : RuleCheckMessage(other) {}
  virtual ~MsgMissingSymbolValue() noexcept {}
};

/*******************************************************************************
 *  Class MsgNonFunctionalSymbolPinInversionSign
 ******************************************************************************/

/**
 * @brief The MsgNonFunctionalSymbolPinInversionSign class
 */
class MsgNonFunctionalSymbolPinInversionSign final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgNonFunctionalSymbolPinInversionSign)

public:
  // Constructors / Destructor
  MsgNonFunctionalSymbolPinInversionSign() = delete;
  explicit MsgNonFunctionalSymbolPinInversionSign(
      std::shared_ptr<const SymbolPin> pin) noexcept;
  MsgNonFunctionalSymbolPinInversionSign(
      const MsgNonFunctionalSymbolPinInversionSign& other) noexcept
    : RuleCheckMessage(other), mPin(other.mPin) {}
  virtual ~MsgNonFunctionalSymbolPinInversionSign() noexcept {}

  // Getters
  const std::shared_ptr<const SymbolPin>& getPin() const noexcept {
    return mPin;
  }

private:
  std::shared_ptr<const SymbolPin> mPin;
};

/*******************************************************************************
 *  Class MsgSymbolOriginNotInCenter
 ******************************************************************************/

/**
 * @brief The MsgSymbolOriginNotInCenter class
 */
class MsgSymbolOriginNotInCenter final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgSymbolOriginNotInCenter)

public:
  // Constructors / Destructor
  MsgSymbolOriginNotInCenter() = delete;
  explicit MsgSymbolOriginNotInCenter(const Point& center) noexcept;
  MsgSymbolOriginNotInCenter(const MsgSymbolOriginNotInCenter& other) noexcept
    : RuleCheckMessage(other), mCenter(other.mCenter) {}
  virtual ~MsgSymbolOriginNotInCenter() noexcept {}

  // Getters
  const Point& getCenter() const noexcept { return mCenter; }

private:
  const Point mCenter;
};

/*******************************************************************************
 *  Class MsgOverlappingSymbolPins
 ******************************************************************************/

/**
 * @brief The MsgOverlappingSymbolPins class
 */
class MsgOverlappingSymbolPins final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgOverlappingSymbolPins)

public:
  // Constructors / Destructor
  MsgOverlappingSymbolPins() = delete;
  MsgOverlappingSymbolPins(
      QVector<std::shared_ptr<const SymbolPin>> pins) noexcept;
  MsgOverlappingSymbolPins(const MsgOverlappingSymbolPins& other) noexcept
    : RuleCheckMessage(other), mPins(other.mPins) {}
  virtual ~MsgOverlappingSymbolPins() noexcept {}

  // Getters
  const QVector<std::shared_ptr<const SymbolPin>>& getPins() const noexcept {
    return mPins;
  }

private:
  static QString buildMessage(
      const QVector<std::shared_ptr<const SymbolPin>>& pins) noexcept;

private:  // Data
  QVector<std::shared_ptr<const SymbolPin>> mPins;
};

/*******************************************************************************
 *  Class MsgSymbolPinNotOnGrid
 ******************************************************************************/

/**
 * @brief The MsgSymbolPinNotOnGrid class
 */
class MsgSymbolPinNotOnGrid final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgSymbolPinNotOnGrid)

public:
  // Constructors / Destructor
  MsgSymbolPinNotOnGrid() = delete;
  MsgSymbolPinNotOnGrid(std::shared_ptr<const SymbolPin> pin,
                        const PositiveLength& gridInterval) noexcept;
  MsgSymbolPinNotOnGrid(const MsgSymbolPinNotOnGrid& other) noexcept
    : RuleCheckMessage(other),
      mPin(other.mPin),
      mGridInterval(other.mGridInterval) {}
  virtual ~MsgSymbolPinNotOnGrid() noexcept {}

  // Getters
  const std::shared_ptr<const SymbolPin>& getPin() const noexcept {
    return mPin;
  }
  const PositiveLength& getGridInterval() const noexcept {
    return mGridInterval;
  }

private:
  std::shared_ptr<const SymbolPin> mPin;
  PositiveLength mGridInterval;
};

/*******************************************************************************
 *  Class MsgWrongSymbolTextLayer
 ******************************************************************************/

/**
 * @brief The MsgWrongSymbolTextLayer class
 */
class MsgWrongSymbolTextLayer final : public RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgWrongSymbolTextLayer)

public:
  // Constructors / Destructor
  MsgWrongSymbolTextLayer() = delete;
  MsgWrongSymbolTextLayer(std::shared_ptr<const Text> text,
                          const Layer& expectedLayer) noexcept;
  MsgWrongSymbolTextLayer(const MsgWrongSymbolTextLayer& other) noexcept
    : RuleCheckMessage(other),
      mText(other.mText),
      mExpectedLayer(other.mExpectedLayer) {}
  virtual ~MsgWrongSymbolTextLayer() noexcept {}

  // Getters
  std::shared_ptr<const Text> getText() const noexcept { return mText; }
  const Layer& getExpectedLayer() const noexcept { return *mExpectedLayer; }

private:
  std::shared_ptr<const Text> mText;
  const Layer* mExpectedLayer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
