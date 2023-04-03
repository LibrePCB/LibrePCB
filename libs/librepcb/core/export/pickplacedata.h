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

#ifndef LIBREPCB_CORE_PICKPLACEDATA_H
#define LIBREPCB_CORE_PICKPLACEDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/angle.h"
#include "../types/point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PickPlaceDataItem
 ******************************************************************************/

/**
 * @brief The ::librepcb::PickPlaceDataItem class represents one item of a
 *        pick&place file
 *
 * @see ::librepcb::PickPlaceData
 */
class PickPlaceDataItem final {
  Q_DECLARE_TR_FUNCTIONS(PickPlaceDataItem)

public:
  enum class BoardSide {
    Top,
    Bottom,
  };

  enum class Type {
    Tht,  ///< Pure THT package
    Smt,  ///< Pure SMT package
    Mixed,  ///< Mixed THT/SMT package
    Fiducial,  ///< No package to mount, just a fiducial
    Other,  ///< Anything special, e.g. mechanical parts
  };

  // Constructors / Destructor
  PickPlaceDataItem() = delete;
  PickPlaceDataItem(const QString& designator, const QString& value,
                    const QString& deviceName, const QString& packageName,
                    const Point& position, const Angle& rotation,
                    BoardSide boardSide, Type type) noexcept
    : mDesignator(designator),
      mValue(value),
      mDeviceName(deviceName),
      mPackageName(packageName),
      mPosition(position),
      mRotation(rotation),
      mBoardSide(boardSide),
      mType(type) {}
  PickPlaceDataItem(const PickPlaceDataItem& other) noexcept
    : mDesignator(other.mDesignator),
      mValue(other.mValue),
      mDeviceName(other.mDeviceName),
      mPackageName(other.mPackageName),
      mPosition(other.mPosition),
      mRotation(other.mRotation),
      mBoardSide(other.mBoardSide),
      mType(other.mType) {}
  ~PickPlaceDataItem() noexcept {}

  // Getters
  const QString& getDesignator() const noexcept { return mDesignator; }
  const QString& getValue() const noexcept { return mValue; }
  const QString& getDeviceName() const noexcept { return mDeviceName; }
  const QString& getPackageName() const noexcept { return mPackageName; }
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  BoardSide getBoardSide() const noexcept { return mBoardSide; }
  Type getType() const noexcept { return mType; }

  // Setters
  void setDesignator(const QString& value) noexcept { mDesignator = value; }

  // Operator Overloadings
  PickPlaceDataItem& operator=(const PickPlaceDataItem& rhs) noexcept {
    mDesignator = rhs.mDesignator;
    mValue = rhs.mValue;
    mDeviceName = rhs.mDeviceName;
    mPackageName = rhs.mPackageName;
    mPosition = rhs.mPosition;
    mRotation = rhs.mRotation;
    mBoardSide = rhs.mBoardSide;
    mType = rhs.mType;
    return *this;
  }

private:
  QString mDesignator;
  QString mValue;
  QString mDeviceName;
  QString mPackageName;
  Point mPosition;
  Angle mRotation;
  BoardSide mBoardSide;
  Type mType;
};

/*******************************************************************************
 *  Class PickPlaceData
 ******************************************************************************/

/**
 * @brief The ::librepcb::PickPlaceData class represents the content of a
 *        pick&place file
 */
class PickPlaceData final {
  Q_DECLARE_TR_FUNCTIONS(PickPlaceData)

public:
  // Constructors / Destructor
  PickPlaceData() = delete;
  PickPlaceData(const PickPlaceData& other) noexcept = delete;
  PickPlaceData(const QString& projectName, const QString& projectVersion,
                const QString& boardName) noexcept;
  ~PickPlaceData() noexcept;

  // Getters
  const QString& getProjectName() const noexcept { return mProjectName; }
  const QString& getProjectVersion() const noexcept { return mProjectVersion; }
  const QString& getBoardName() const noexcept { return mBoardName; }
  const QList<PickPlaceDataItem>& getItems() const noexcept { return mItems; }

  // General Methods
  void addItem(const PickPlaceDataItem& item) noexcept;

  // Operator Overloadings
  PickPlaceData& operator=(const PickPlaceData& rhs) noexcept = delete;

private:
  QString mProjectName;
  QString mProjectVersion;
  QString mBoardName;
  QList<PickPlaceDataItem> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
