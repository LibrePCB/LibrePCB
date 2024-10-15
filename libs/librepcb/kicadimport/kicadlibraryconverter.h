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

#ifndef LIBREPCB_KICADIMPORT_KICADLIBRARYCONVERTER_H
#define LIBREPCB_KICADIMPORT_KICADLIBRARYCONVERTER_H

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
namespace librepcb {

class Component;
class Device;
class MessageLogger;
class Package;
class Polygon;
class Symbol;

namespace kicadimport {

/*******************************************************************************
 *  Struct KiCadLibraryConverterSettings
 ******************************************************************************/

/**
 * @brief Settings for ::librepcb::kicadimport::KiCadLibraryConverter
 */
struct KiCadLibraryConverterSettings final {
  KiCadLibraryConverterSettings() noexcept;

  // QString namePrefix;
  // Version version;
  QString author;
  QString keywords;
  QSet<Uuid> symbolCategories;
  QSet<Uuid> packageCategories;
  QSet<Uuid> componentCategories;
  QSet<Uuid> deviceCategories;
  // BoundedUnsignedRatio autoThtAnnularWidth;
};

/*******************************************************************************
 *  Class KiCadLibraryConverter
 ******************************************************************************/

/**
 * @brief Converts KiCad library elements to LibrePCB
 */
class KiCadLibraryConverter final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  KiCadLibraryConverter(const KiCadLibraryConverter& other) = delete;
  explicit KiCadLibraryConverter(const KiCadLibraryConverterSettings& settings,
                                 QObject* parent = nullptr) noexcept;
  ~KiCadLibraryConverter() noexcept;

  // Getters
  Uuid getComponentSignalOfSymbolPin(const QString& libName,
                                     const QString& libUrn,
                                     const QString& devSetName,
                                     const QString& gateName,
                                     const QString& pinName) const;

  // General Methods
  void reset() noexcept;
  // std::unique_ptr<Symbol> createSymbol(const QString& libName,
  //                                      const QString& libUrn,
  //                                      const parseagle::Symbol& eagleSymbol,
  //                                      MessageLogger& log);
  // std::unique_ptr<Package> createPackage(const QString& libName,
  //                                        const QString& libUrn,
  //                                        const parseagle::Package&
  //                                        eaglePackage, MessageLogger& log);
  // std::unique_ptr<Component> createComponent(
  //     const QString& libName, const QString& libUrn,
  //     const parseagle::DeviceSet& eagleDeviceSet, MessageLogger& log);
  // std::unique_ptr<Device> createDevice(
  //     const QString& devLibName, const QString& devLibUrn,
  //     const parseagle::DeviceSet& eagleDeviceSet,
  //     const parseagle::Device& eagleDevice, const QString& pkgLibName,
  //     const QString& pkgLibUrn, MessageLogger& log);

  // Operator Overloadings
  KiCadLibraryConverter& operator=(const KiCadLibraryConverter& rhs) = delete;

private:  // Methods
  void tryOrLogError(std::function<void()> func, MessageLogger& log);

private:  // Data
  KiCadLibraryConverterSettings mSettings;

  // State

  /// Key: [Library Name, Library URN, Symbol Name]
  /// Value: LibrePCB Symbol UUID
  QHash<QStringList, tl::optional<Uuid> > mSymbolMap;

  /// Key: [Library Name, Library URN, Symbol Name] | Pin Name
  /// Value: (KiCad Pin Object, LibrePCB Symbol Pin UUID)
  // QHash<QStringList,
  //       QMap<QString,
  //            std::pair<std::shared_ptr<parseagle::Pin>, tl::optional<Uuid> >
  //            > >
  //     mSymbolPinMap;

  /// Key: [Library Name, Library URN, Package Name]
  /// Value: LibrePCB Package UUID
  QHash<QStringList, tl::optional<Uuid> > mPackageMap;

  /// Key: [Library Name, Library URN, Package Name] | Pad Name
  /// Value: LibrePCB Package Pad UUID
  QHash<QStringList, QMap<QString, tl::optional<Uuid> > > mPackagePadMap;

  /// Key: [Library Name, Library URN, Device Set Name]
  /// Value: LibrePCB Component UUID
  QHash<QStringList, tl::optional<Uuid> > mComponentMap;

  /// Key: [Library Name, Library URN, Device Set Name, Gate Name, Pin Name]
  /// Value: LibrePCB Component Signal UUID
  QHash<QStringList, tl::optional<Uuid> > mComponentSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
