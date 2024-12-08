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

#ifndef LIBREPCB_EAGLEIMPORT_EAGLEPROJECTIMPORT_H
#define LIBREPCB_EAGLEIMPORT_EAGLEPROJECTIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/boundedunsignedratio.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace parseagle {
class Board;
class Device;
class DeviceSet;
class Library;
class Net;
class Package;
class Part;
class Schematic;
class Sheet;
class Symbol;
class Technology;
}  // namespace parseagle

namespace librepcb {

class Board;
class Component;
class Device;
class FilePath;
class MessageLogger;
class NetSignal;
class Package;
class Project;
class ProjectLibrary;
class Schematic;
class Symbol;

namespace eagleimport {

class EagleLibraryConverter;

/*******************************************************************************
 *  Class EagleProjectImport
 ******************************************************************************/

/**
 * @brief Loads and imports an EAGLE project into a ::librepcb::Project
 */
class EagleProjectImport final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  EagleProjectImport(const EagleProjectImport& other) = delete;
  explicit EagleProjectImport(QObject* parent = nullptr) noexcept;
  ~EagleProjectImport() noexcept;

  // Getters
  std::shared_ptr<MessageLogger> getLogger() const noexcept { return mLogger; }
  bool isReady() const noexcept { return !mSchematic.isNull(); }
  const QString& getProjectName() const noexcept { return mProjectName; }
  int getSheetCount() const noexcept;
  bool hasBoard() const noexcept { return !mBoard.isNull(); }

  // General Methods
  void reset() noexcept;
  QStringList open(const FilePath& sch, const FilePath& brd);
  void import(Project& project);

  // Operator Overloadings
  EagleProjectImport& operator=(const EagleProjectImport& rhs) = delete;

private:  // Methods
  const Symbol& importLibrarySymbol(EagleLibraryConverter& converter,
                                    ProjectLibrary& library,
                                    const QString& libName,
                                    const QString& libUrn,
                                    const QString& symName);
  const Component& importLibraryComponent(EagleLibraryConverter& converter,
                                          ProjectLibrary& library,
                                          const QString& libName,
                                          const QString& libUrn,
                                          const QString& devSetName);
  const Package& importLibraryPackage(EagleLibraryConverter& converter,
                                      ProjectLibrary& library,
                                      const QString& libName,
                                      const QString& libUrn,
                                      const QString& pkgName);
  const Device& importLibraryDevice(
      EagleLibraryConverter& converter, ProjectLibrary& library,
      const QString& devLibName, const QString& devLibUrn,
      const QString& devSetName, const QString& devName,
      const QString& pkgLibName, const QString& pkgLibUrn);
  NetSignal& importNet(Project& project, const parseagle::Net& net);
  void importLibraries(const QList<parseagle::Library>& libs, bool isBoard);
  void importSchematic(Project& project, EagleLibraryConverter& converter,
                       const parseagle::Sheet& sheet);
  void importBoard(Project& project, EagleLibraryConverter& converter);
  bool hasBuses(const parseagle::Schematic& schematic) const noexcept;
  std::optional<BoundedUnsignedRatio> tryGetDrcRatio(const QString& nr,
                                                     const QString& nmin,
                                                     const QString& nmax) const;
  std::shared_ptr<const parseagle::Symbol> getSymbol(const QString& libName,
                                                     const QString& libUrn,
                                                     const QString& name) const;
  std::shared_ptr<const parseagle::Package> getPackage(
      const QString& libName, const QString& libUrn, const QString& name) const;
  std::shared_ptr<const parseagle::DeviceSet> getDeviceSet(
      const QString& libName, const QString& libUrn, const QString& name) const;
  const parseagle::Device& getDevice(const parseagle::DeviceSet& devSet,
                                     const QString& name) const;
  const parseagle::Technology* tryGetTechnology(const parseagle::Device& dev,
                                                const QString& name) const;
  const parseagle::Part& getPart(const QString& name) const;

private:  // Data
  std::shared_ptr<MessageLogger> mLogger;
  QString mProjectName;
  QScopedPointer<parseagle::Schematic> mSchematic;
  QScopedPointer<parseagle::Board> mBoard;

  /// Key={libName, libUrn, symName}
  QHash<QStringList, std::shared_ptr<const parseagle::Symbol>> mSymbols;

  /// Key={libName, libUrn, pkgName}
  QHash<QStringList, std::shared_ptr<const parseagle::Package>> mPackages;

  /// Key={libName, libUrn, devSetName}
  QHash<QStringList, std::shared_ptr<const parseagle::DeviceSet>> mDeviceSets;

  /// Key={libName, libUrn, symName}, Value=libSymUuid
  QHash<QStringList, Uuid> mLibSymbolMap;

  /// Key={libName, libUrn, deviceSetName}, Value=libCmpUuid
  QHash<QStringList, Uuid> mLibComponentMap;

  /// Key={libCmpUuid, gateName}, Value=libCmpGateUuid
  QHash<QStringList, Uuid> mLibComponentGateMap;

  /// Key={libName, libUrn, pkgName}, Value=libPkgUuid
  QHash<QStringList, Uuid> mLibPackageMap;

  /// Key={libName, libUrn, devSetname, devName}, Value=libDevUuid
  QHash<QStringList, Uuid> mLibDeviceMap;

  struct ComponentMap {
    QString libName;
    QString libUrn;
    QString devSetName;
    QString devName;
    Uuid uuid;
  };
  QHash<QString, ComponentMap> mComponentMap;

  /// All already imported schematic directory names
  QSet<QString> mSchematicDirNames;

  /// Key=eagleNetName, Value=netSignalUuid
  QHash<QString, Uuid> mNetSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif
