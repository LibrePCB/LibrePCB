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
#include <optional/tl/optional.hpp>

#include <QtCore>

#include <memory>

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
                                     const QString& devSetName,
                                     const QString& gateName,
                                     const QString& pinName) const;

  // General Methods
  void reset() noexcept;
  std::unique_ptr<Symbol> createSymbol(const QString& libName,
                                       const parseagle::Symbol& eagleSymbol);
  std::unique_ptr<Package> createPackage(
      const QString& libName, const parseagle::Package& eaglePackage);
  std::unique_ptr<Component> createComponent(
      const QString& libName, const parseagle::DeviceSet& eagleDeviceSet);
  std::unique_ptr<Device> createDevice(
      const QString& libName, const parseagle::DeviceSet& eagleDeviceSet,
      const parseagle::Device& eagleDevice);

  // Operator Overloadings
  EagleLibraryConverter& operator=(const EagleLibraryConverter& rhs) = delete;

signals:
  void errorOccurred(const QString& elementName, const QString& msg);

private:  // Methods
  void tryOrRaiseError(const QString& element, std::function<void()> func);

private:  // Data
  EagleLibraryConverterSettings mSettings;

  // State

  /// Key: (Library Name, Symbol Name)
  /// Value: LibrePCB Symbol UUID
  QHash<std::pair<QString, QString>, tl::optional<Uuid> > mSymbolMap;

  /// Key: (Library Name, Symbol Name) | Pin Name
  /// Value: (EAGLE Pin Object, LibrePCB Symbol Pin UUID)
  QHash<
      std::pair<QString, QString>,
      QHash<QString,
            std::pair<std::shared_ptr<parseagle::Pin>, tl::optional<Uuid> > > >
      mSymbolPinMap;

  /// Key: (Library Name, Package Name)
  /// Value: LibrePCB Package UUID
  QHash<std::pair<QString, QString>, tl::optional<Uuid> > mPackageMap;

  /// Key: (Library Name, Package Name) | Pad Name
  /// Value: LibrePCB Package Pad UUID
  QHash<std::pair<QString, QString>, QHash<QString, tl::optional<Uuid> > >
      mPackagePadMap;

  /// Key: (Library Name, Device Set Name)
  /// Value: LibrePCB Component UUID
  QHash<std::pair<QString, QString>, tl::optional<Uuid> > mComponentMap;

  /// Key: (Library Name, Device Set Name) | Gate Name | Pin Name
  /// Value: LibrePCB Component Signal UUID
  QHash<std::pair<QString, QString>,
        QHash<QString, QHash<QString, tl::optional<Uuid> > > >
      mComponentSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif
