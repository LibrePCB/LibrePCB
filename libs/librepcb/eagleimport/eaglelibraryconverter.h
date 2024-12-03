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

#ifndef LIBREPCB_EAGLEIMPORT_EAGLELIBRARYCONVERTER_H
#define LIBREPCB_EAGLEIMPORT_EAGLELIBRARYCONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/boundedunsignedratio.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace parseagle {
class Device;
class DeviceSet;
class Package;
class Pin;
class Symbol;
}  // namespace parseagle

namespace librepcb {

class Component;
class Device;
class MessageLogger;
class Package;
class Polygon;
class Symbol;

namespace eagleimport {

/*******************************************************************************
 *  Struct EagleLibraryConverterSettings
 ******************************************************************************/

/**
 * @brief Settings for ::librepcb::eagleimport::EagleLibraryConverter
 */
struct EagleLibraryConverterSettings final {
  EagleLibraryConverterSettings() noexcept;

  QString namePrefix;
  Version version;
  QString author;
  QString keywords;
  QSet<Uuid> symbolCategories;
  QSet<Uuid> packageCategories;
  QSet<Uuid> componentCategories;
  QSet<Uuid> deviceCategories;
  BoundedUnsignedRatio autoThtAnnularWidth;
};

/*******************************************************************************
 *  Class EagleLibraryConverter
 ******************************************************************************/

/**
 * @brief Converts EAGLE library elements to LibrePCB
 */
class EagleLibraryConverter final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  EagleLibraryConverter(const EagleLibraryConverter& other) = delete;
  explicit EagleLibraryConverter(const EagleLibraryConverterSettings& settings,
                                 QObject* parent = nullptr) noexcept;
  ~EagleLibraryConverter() noexcept;

  // Getters
  Uuid getComponentSignalOfSymbolPin(const QString& libName,
                                     const QString& libUrn,
                                     const QString& devSetName,
                                     const QString& gateName,
                                     const QString& pinName) const;

  // General Methods
  void reset() noexcept;
  std::unique_ptr<Symbol> createSymbol(const QString& libName,
                                       const QString& libUrn,
                                       const parseagle::Symbol& eagleSymbol,
                                       MessageLogger& log);
  std::unique_ptr<Package> createPackage(const QString& libName,
                                         const QString& libUrn,
                                         const parseagle::Package& eaglePackage,
                                         MessageLogger& log);
  std::unique_ptr<Component> createComponent(
      const QString& libName, const QString& libUrn,
      const parseagle::DeviceSet& eagleDeviceSet, MessageLogger& log);
  std::unique_ptr<Device> createDevice(
      const QString& devLibName, const QString& devLibUrn,
      const parseagle::DeviceSet& eagleDeviceSet,
      const parseagle::Device& eagleDevice, const QString& pkgLibName,
      const QString& pkgLibUrn, MessageLogger& log);

  // Operator Overloadings
  EagleLibraryConverter& operator=(const EagleLibraryConverter& rhs) = delete;

private:  // Methods
  void tryOrLogError(std::function<void()> func, MessageLogger& log);

private:  // Data
  EagleLibraryConverterSettings mSettings;

  // State

  /// Key: [Library Name, Library URN, Symbol Name]
  /// Value: LibrePCB Symbol UUID
  QHash<QStringList, std::optional<Uuid> > mSymbolMap;

  /// Key: [Library Name, Library URN, Symbol Name] | Pin Name
  /// Value: (EAGLE Pin Object, LibrePCB Symbol Pin UUID)
  QHash<
      QStringList,
      QMap<QString,
           std::pair<std::shared_ptr<parseagle::Pin>, std::optional<Uuid> > > >
      mSymbolPinMap;

  /// Key: [Library Name, Library URN, Package Name]
  /// Value: LibrePCB Package UUID
  QHash<QStringList, std::optional<Uuid> > mPackageMap;

  /// Key: [Library Name, Library URN, Package Name] | Pad Name
  /// Value: LibrePCB Package Pad UUID
  QHash<QStringList, QMap<QString, std::optional<Uuid> > > mPackagePadMap;

  /// Key: [Library Name, Library URN, Device Set Name]
  /// Value: LibrePCB Component UUID
  QHash<QStringList, std::optional<Uuid> > mComponentMap;

  /// Key: [Library Name, Library URN, Device Set Name, Gate Name, Pin Name]
  /// Value: LibrePCB Component Signal UUID
  QHash<QStringList, std::optional<Uuid> > mComponentSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif
