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

#ifndef LIBREPCB_EAGLEIMPORT_EAGLELIBRARYIMPORT_H
#define LIBREPCB_EAGLEIMPORT_EAGLELIBRARYIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace parseagle {
class Device;
class DeviceSet;
class Package;
class Symbol;
class Wire;
}  // namespace parseagle

namespace librepcb {

class Polygon;

namespace eagleimport {

/*******************************************************************************
 *  Class EagleLibraryImport
 ******************************************************************************/

/**
 * @brief EAGLE library (*.lbr) import
 */
class EagleLibraryImport final : public QThread {
  Q_OBJECT

public:
  struct Symbol {
    QString displayName;
    QString description;
    Qt::CheckState checkState;
    std::shared_ptr<parseagle::Symbol> symbol;
  };

  struct Package {
    QString displayName;
    QString description;
    Qt::CheckState checkState;
    std::shared_ptr<parseagle::Package> package;
  };

  struct Component {
    QString displayName;
    QString description;
    Qt::CheckState checkState;
    QSet<QString> symbolDisplayNames;
    std::shared_ptr<parseagle::DeviceSet> deviceSet;
  };

  struct Device {
    QString displayName;
    QString description;
    Qt::CheckState checkState;
    QString componentDisplayName;
    QString packageDisplayName;
    std::shared_ptr<parseagle::Device> device;
    std::shared_ptr<parseagle::DeviceSet> deviceSet;
  };

  // Constructors / Destructor
  EagleLibraryImport(const EagleLibraryImport& other) = delete;
  EagleLibraryImport(const FilePath& dstLibFp,
                     QObject* parent = nullptr) noexcept;
  ~EagleLibraryImport() noexcept;

  // Getters
  const FilePath& getLoadedFilePath() const noexcept { return mLoadedFilePath; }
  int getTotalElementsCount() const noexcept;
  int getCheckedElementsCount() const noexcept;
  int getCheckedSymbolsCount() const noexcept;
  int getCheckedPackagesCount() const noexcept;
  int getCheckedComponentsCount() const noexcept;
  int getCheckedDevicesCount() const noexcept;
  const QVector<Symbol>& getSymbols() const noexcept { return mSymbols; }
  const QVector<Package>& getPackages() const noexcept { return mPackages; }
  const QVector<Component>& getComponents() const noexcept {
    return mComponents;
  }
  const QVector<Device>& getDevices() const noexcept { return mDevices; }

  // Setters
  void setNamePrefix(const QString& prefix) noexcept { mNamePrefix = prefix; }
  void setSymbolCategories(const QSet<Uuid>& uuids) noexcept {
    mSymbolCategories = uuids;
  }
  void setPackageCategories(const QSet<Uuid>& uuids) noexcept {
    mPackageCategories = uuids;
  }
  void setComponentCategories(const QSet<Uuid>& uuids) noexcept {
    mComponentCategories = uuids;
  }
  void setDeviceCategories(const QSet<Uuid>& uuids) noexcept {
    mDeviceCategories = uuids;
  }
  void setSymbolChecked(const QString& name, bool checked) noexcept;
  void setPackageChecked(const QString& name, bool checked) noexcept;
  void setComponentChecked(const QString& name, bool checked) noexcept;
  void setDeviceChecked(const QString& name, bool checked) noexcept;

  // General Methods
  void reset() noexcept;
  QStringList open(const FilePath& lbr);

  // Operator Overloadings
  EagleLibraryImport& operator=(const EagleLibraryImport& rhs) = delete;

signals:
  void symbolCheckStateChanged(const QString& name, Qt::CheckState state);
  void packageCheckStateChanged(const QString& name, Qt::CheckState state);
  void componentCheckStateChanged(const QString& name, Qt::CheckState state);
  void progressStatus(const QString& status);
  void progressPercent(int percent);
  void errorOccurred(const QString& error);
  void finished(const QStringList& errors);

private:  // Methods
  template <typename T>
  int getCheckedElementsCount(const QVector<T>& elements) const noexcept;
  template <typename T>
  void setElementChecked(QVector<T>& elements, const QString& name,
                         bool checked) noexcept;
  void updateDependencies() noexcept;
  template <typename T>
  bool setElementDependent(T& element, bool dependent) noexcept;
  QVector<std::shared_ptr<Polygon> > convertWires(
      const QString& element, const QList<parseagle::Wire>& wires);
  void tryOrRaiseError(const QString& element, std::function<void()> func);
  void raiseImportError(const QString& element, const QString& error) noexcept;
  void run() noexcept override;

private:  // Data
  // Configuration
  FilePath mDestinationLibraryFp;
  QString mNamePrefix;
  Version mVersion;
  QString mAuthor;
  QString mKeywords;
  QSet<Uuid> mSymbolCategories;
  QSet<Uuid> mPackageCategories;
  QSet<Uuid> mComponentCategories;
  QSet<Uuid> mDeviceCategories;

  // State
  bool mAbort;
  FilePath mLoadedFilePath;
  QStringList mImportErrors;

  // Library elements
  QVector<Symbol> mSymbols;
  QVector<Package> mPackages;
  QVector<Component> mComponents;
  QVector<Device> mDevices;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif
