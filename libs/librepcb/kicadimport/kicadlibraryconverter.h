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
#include "kicadtypes.h"

#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Device;
class FilePath;
class Package;
class Symbol;
class WorkspaceLibraryDb;

namespace kicadimport {

/*******************************************************************************
 *  Struct KiCadLibraryConverterSettings
 ******************************************************************************/

/**
 * @brief Settings for ::librepcb::kicadimport::KiCadLibraryConverter
 */
struct KiCadLibraryConverterSettings final {
  KiCadLibraryConverterSettings() noexcept;

  QString namePrefix;
  Version version;
  QString author;
  QString keywords;
  QSet<Uuid> symbolCategories;
  QSet<Uuid> packageCategories;
  QSet<Uuid> componentCategories;
  QSet<Uuid> deviceCategories;
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
  explicit KiCadLibraryConverter(WorkspaceLibraryDb& db,
                                 const KiCadLibraryConverterSettings& settings,
                                 QObject* parent = nullptr) noexcept;
  ~KiCadLibraryConverter() noexcept;

  // General Methods
  void reset() noexcept;
  std::unique_ptr<Package> createPackage(const FilePath& libFp,
                                         const KiCadFootprint& kiFpt,
                                         const QString& generatedBy,
                                         const QMap<QString, FilePath>& models,
                                         MessageLogger& log);
  std::unique_ptr<Symbol> createSymbol(const FilePath& libFp,
                                       const KiCadSymbol& kiSym,
                                       const KiCadSymbolGate& kiGate,
                                       const QString& generatedBy,
                                       MessageLogger& log);
  std::unique_ptr<Component> createComponent(
      const FilePath& libFp, const KiCadSymbol& kiSym,
      const QList<KiCadSymbolGate>& kiGates, const QString& generatedBy,
      const QStringList& symGeneratedBy, MessageLogger& log);
  std::unique_ptr<Device> createDevice(const FilePath& libFp,
                                       const KiCadSymbol& kiSym,
                                       const QList<KiCadSymbolGate>& kiGates,
                                       const QString& generatedBy,
                                       const QString& cmpGeneratedBy,
                                       QString pkgGeneratedBy,
                                       MessageLogger& log);

  // Operator Overloadings
  KiCadLibraryConverter& operator=(const KiCadLibraryConverter& rhs) = delete;

private:  // Methods
  void loadAlreadyImportedSymbol(const QString& generatedBy);
  QString loadAlreadyImportedPackage(const QStringList& generatedBy);
  void loadAlreadyImportedComponent(const QString& generatedBy);
  template <typename T>
  FilePath getAlreadyImportedFp(const QStringList& generatedBy) const;
  void tryOrLogError(std::function<void()> func, MessageLogger& log);

private:  // Data
  WorkspaceLibraryDb& mLibraryDb;
  KiCadLibraryConverterSettings mSettings;

  /// Key: generatedBy
  /// Value: LibrePCB Package UUID
  QHash<QString, std::optional<Uuid>> mPackageMap;

  /// Key: generatedBy | Pad Number (after conversion)
  /// Value: LibrePCB Package Pad UUID
  QHash<QString, QMap<QString, std::optional<Uuid>>> mPackagePadMap;

  /// Key: generatedBy
  /// Value: LibrePCB Symbol UUID
  QHash<QString, std::optional<Uuid>> mSymbolMap;

  /// Key: [Symbol generatedBy, Pin Name (after conversion)]
  /// Value: LibrePCB Symbol Pin UUID
  QHash<std::pair<QString, QString>, std::optional<Uuid>> mSymbolPinMap;

  /// Key: generatedBy
  /// Value: LibrePCB Component UUID
  QHash<QString, std::optional<Uuid>> mComponentMap;

  /// Key: [Component generatedBy, Signal Name (after conversion)]
  /// Value: LibrePCB Component Signal UUID
  QHash<std::pair<QString, QString>, std::optional<Uuid>> mComponentSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
