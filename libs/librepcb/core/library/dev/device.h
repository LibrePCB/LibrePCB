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

#ifndef LIBREPCB_CORE_DEVICE_H
#define LIBREPCB_CORE_DEVICE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../attribute/attribute.h"
#include "../libraryelement.h"
#include "devicepadsignalmap.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Device
 ******************************************************************************/

/**
 * @brief The Device class represents an instance of a component (a "real"
 * component)
 *
 * Following information is considered as the "interface" of a device and must
 * therefore never be changed:
 *  - UUID
 *  - Component UUID
 *  - Package UUID
 *  - Pad-signal-mapping
 */
class Device final : public LibraryElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  Device() = delete;
  Device(const Device& other) = delete;
  Device(const Uuid& uuid, const Version& version, const QString& author,
         const ElementName& name_en_US, const QString& description_en_US,
         const QString& keywords_en_US, const Uuid& component,
         const Uuid& package);
  ~Device() noexcept;

  // Getters
  const Uuid& getComponentUuid() const noexcept { return mComponentUuid; }
  const Uuid& getPackageUuid() const noexcept { return mPackageUuid; }
  const AttributeList& getAttributes() const noexcept { return mAttributes; }
  DevicePadSignalMap& getPadSignalMap() noexcept { return mPadSignalMap; }
  const DevicePadSignalMap& getPadSignalMap() const noexcept {
    return mPadSignalMap;
  }

  // Setters
  void setComponentUuid(const Uuid& uuid) noexcept;
  void setPackageUuid(const Uuid& uuid) noexcept;

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const override;

  // Operator Overloadings
  Device& operator=(const Device& rhs) = delete;

  // Static Methods
  static std::unique_ptr<Device> open(
      std::unique_ptr<TransactionalDirectory> directory);
  static QString getShortElementName() noexcept {
    return QStringLiteral("dev");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("device");
  }

signals:
  void componentUuidChanged(const Uuid& uuid);
  void packageUuidChanged(const Uuid& uuid);

protected:  // Methods
  virtual void serialize(SExpression& root) const override;

private:  // Methods
  Device(std::unique_ptr<TransactionalDirectory> directory,
         const SExpression& root);

private:  // Data
  Uuid mComponentUuid;
  Uuid mPackageUuid;
  AttributeList
      mAttributes;  ///< not yet used, but already specified in file format
  DevicePadSignalMap mPadSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
