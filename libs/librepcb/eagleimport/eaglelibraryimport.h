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

class MessageLogger;

namespace eagleimport {

struct EagleLibraryConverterSettings;

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
    QString displayName;  // Same as symbol->getName()
    QString description;  // Same as symbol->getDescription()
    Qt::CheckState checkState;
    std::shared_ptr<parseagle::Symbol> symbol;
  };

  struct Package {
    QString displayName;  // Same as package->getName()
    QString description;  // Same as package->getDescription()
    Qt::CheckState checkState;
    std::shared_ptr<parseagle::Package> package;
  };

  struct Component {
    QString displayName;  // Like deviceSet->getName() but without trailing [-_]
    QString description;  // Same as deviceSet->getDescription()
    Qt::CheckState checkState;
    QSet<QString> symbolDisplayNames;
    std::shared_ptr<parseagle::DeviceSet> deviceSet;
  };

  struct Device {
    QString displayName;  // Built from names of deviceSet and device
    QString description;  // Same as deviceSet->getDescription()
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
  std::shared_ptr<MessageLogger> getLogger() const noexcept { return mLogger; }
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
  void setNamePrefix(const QString& prefix) noexcept;
  void setSymbolCategories(const QSet<Uuid>& uuids) noexcept;
  void setPackageCategories(const QSet<Uuid>& uuids) noexcept;
  void setComponentCategories(const QSet<Uuid>& uuids) noexcept;
  void setDeviceCategories(const QSet<Uuid>& uuids) noexcept;
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
  void finished();

private:  // Methods
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
  QScopedPointer<EagleLibraryConverterSettings> mSettings;
  std::shared_ptr<MessageLogger> mLogger;

  // State
  bool mAbort;
  FilePath mLoadedFilePath;

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
