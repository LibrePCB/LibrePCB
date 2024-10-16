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

#ifndef LIBREPCB_KICADIMPORT_KICADLIBRARYIMPORT_H
#define LIBREPCB_KICADIMPORT_KICADLIBRARYIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class MessageLogger;

namespace kicadimport {

struct KiCadLibraryConverterSettings;

/*******************************************************************************
 *  Class KiCadLibraryImport
 ******************************************************************************/

/**
 * @brief KiCad library (*.lbr) import
 */
class KiCadLibraryImport final : public QThread {
  Q_OBJECT

public:
  struct Symbol {
    QString name;
    QString footprint;  // LIBNAME:FOOTPRINTNAME (optional)
    Qt::CheckState checkState;
  };

  struct SymbolLibrary {
    QString name;
    Qt::CheckState checkState;
    QList<Symbol> symbols;
  };

  struct Footprint {
    QString name;
    Qt::CheckState checkState;
  };

  struct FootprintLibrary {
    QString name;
    Qt::CheckState checkState;
    QList<Footprint> footprints;
  };

  // Constructors / Destructor
  KiCadLibraryImport(const KiCadLibraryImport& other) = delete;
  KiCadLibraryImport(const FilePath& dstLibFp,
                     QObject* parent = nullptr) noexcept;
  ~KiCadLibraryImport() noexcept;

  // Getters
  std::shared_ptr<MessageLogger> getLogger() const noexcept { return mLogger; }
  const FilePath& getLoadedFilePath() const noexcept { return mLoadedFilePath; }
  // int getCheckedElementsCount() const noexcept;
  // int getCheckedSymbolsCount() const noexcept;
  // int getCheckedPackagesCount() const noexcept;
  // int getCheckedComponentsCount() const noexcept;
  // int getCheckedDevicesCount() const noexcept;
  const QList<SymbolLibrary>& getSymbolLibraries() const noexcept {
    return mSymbolLibs;
  }
  const QList<FootprintLibrary>& getFootprintLibraries() const noexcept {
    return mFootprintLibs;
  }
  // const QVector<Component>& getComponents() const noexcept {
  //   return mComponents;
  // }
  // const QVector<Device>& getDevices() const noexcept { return mDevices; }

  // Setters
  // void setSymbolCategories(const QSet<Uuid>& uuids) noexcept;
  // void setPackageCategories(const QSet<Uuid>& uuids) noexcept;
  // void setComponentCategories(const QSet<Uuid>& uuids) noexcept;
  // void setDeviceCategories(const QSet<Uuid>& uuids) noexcept;
  // void setSymbolChecked(const QString& name, bool checked) noexcept;
  // void setPackageChecked(const QString& name, bool checked) noexcept;
  // void setComponentChecked(const QString& name, bool checked) noexcept;
  // void setDeviceChecked(const QString& name, bool checked) noexcept;

  // General Methods
  void reset() noexcept;
  void open(const FilePath& dir, MessageLogger& log);

  // Operator Overloadings
  KiCadLibraryImport& operator=(const KiCadLibraryImport& rhs) = delete;

signals:
  void symbolCheckStateChanged(const QString& name, Qt::CheckState state);
  void packageCheckStateChanged(const QString& name, Qt::CheckState state);
  void componentCheckStateChanged(const QString& name, Qt::CheckState state);
  void progressStatus(const QString& status);
  void progressPercent(int percent);
  void finished();

private:  // Methods
  void openImpl(const FilePath& dir, MessageLogger& log);
  template <typename T>
  int getCheckedElementsCount(const QVector<T>& elements) const noexcept;
  template <typename T>
  void setElementChecked(QVector<T>& elements, const QString& name,
                         bool checked) noexcept;
  void updateDependencies() noexcept;
  template <typename T>
  bool setElementDependent(T& element, bool dependent) noexcept;
  void run() noexcept override;

private:  // Data
  const FilePath mDestinationLibraryFp;
  QScopedPointer<KiCadLibraryConverterSettings> mSettings;
  std::shared_ptr<MessageLogger> mLogger;

  // State
  bool mAbort;
  FilePath mLoadedFilePath;

  // Library elements
  QList<SymbolLibrary> mSymbolLibs;
  QList<FootprintLibrary> mFootprintLibs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
