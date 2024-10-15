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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "kicadlibraryimport.h"

#include "kicadlibraryconverter.h"
#include "kicadtypeconverter.h"
#include "kicadtypes.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/messagelogger.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace kicadimport {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryImport::KiCadLibraryImport(const FilePath& dstLibFp,
                                       QObject* parent) noexcept
  : QThread(parent),
    mDestinationLibraryFp(dstLibFp),
    mSettings(new KiCadLibraryConverterSettings()),
    mLogger(new MessageLogger(true)),
    mAbort(false) {
}

KiCadLibraryImport::~KiCadLibraryImport() noexcept {
  // Deleting a QThread while it is running will lead to a crash.
  mAbort = true;
  wait();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int KiCadLibraryImport::getTotalElementsCount() const noexcept {
  return mSymbols.count() + mPackages.count() + mComponents.count() +
      mDevices.count();
}

/*int KiCadLibraryImport::getCheckedElementsCount() const noexcept {
  return getCheckedSymbolsCount() + getCheckedPackagesCount() +
      getCheckedComponentsCount() + getCheckedDevicesCount();
}

int KiCadLibraryImport::getCheckedSymbolsCount() const noexcept {
  return getCheckedElementsCount(mSymbols);
}

int KiCadLibraryImport::getCheckedPackagesCount() const noexcept {
  return getCheckedElementsCount(mPackages);
}

int KiCadLibraryImport::getCheckedComponentsCount() const noexcept {
  return getCheckedElementsCount(mComponents);
}

int KiCadLibraryImport::getCheckedDevicesCount() const noexcept {
  return getCheckedElementsCount(mDevices);
}*/

/*******************************************************************************
 *  Setters
 ******************************************************************************/

/*void KiCadLibraryImport::setSymbolCategories(const QSet<Uuid>& uuids) noexcept
{ mSettings->symbolCategories = uuids;
}

void KiCadLibraryImport::setPackageCategories(
    const QSet<Uuid>& uuids) noexcept {
  mSettings->packageCategories = uuids;
}

void KiCadLibraryImport::setComponentCategories(
    const QSet<Uuid>& uuids) noexcept {
  mSettings->componentCategories = uuids;
}

void KiCadLibraryImport::setDeviceCategories(const QSet<Uuid>& uuids) noexcept {
  mSettings->deviceCategories = uuids;
}

void KiCadLibraryImport::setSymbolChecked(const QString& name,
                                          bool checked) noexcept {
  setElementChecked(mSymbols, name, checked);
}

void KiCadLibraryImport::setPackageChecked(const QString& name,
                                           bool checked) noexcept {
  setElementChecked(mPackages, name, checked);
}

void KiCadLibraryImport::setComponentChecked(const QString& name,
                                             bool checked) noexcept {
  setElementChecked(mComponents, name, checked);
}

void KiCadLibraryImport::setDeviceChecked(const QString& name,
                                          bool checked) noexcept {
  setElementChecked(mDevices, name, checked);
}*/

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImport::reset() noexcept {
  mSymbols.clear();
  mPackages.clear();
  mComponents.clear();
  mDevices.clear();
  mLoadedFilePath = FilePath();
}

QStringList KiCadLibraryImport::open(const FilePath& dir) {
  reset();

  // Scan directory for libraries.
  QStringList errors;
  open(dir, errors);

  // Scan subdirectories for libraries (not recursive).
  QDir qDir(dir.toStr());
  qDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  foreach (const QFileInfo& info, qDir.entryInfoList()) {
    open(FilePath{info.absoluteFilePath()}, errors);
  }

  // Sort all elements by name to improve readability.
  /*Toolbox::sortNumeric(
      mSymbols,
      [](const QCollator& cmp, const Symbol& lhs, const Symbol& rhs) {
        return cmp(lhs.displayName, rhs.displayName);
      },
      Qt::CaseInsensitive, false);
  Toolbox::sortNumeric(
      mPackages,
      [](const QCollator& cmp, const Package& lhs, const Package& rhs) {
        return cmp(lhs.displayName, rhs.displayName);
      },
      Qt::CaseInsensitive, false);
  Toolbox::sortNumeric(
      mComponents,
      [](const QCollator& cmp, const Component& lhs, const Component& rhs) {
        return cmp(lhs.displayName, rhs.displayName);
      },
      Qt::CaseInsensitive, false);
  Toolbox::sortNumeric(
      mDevices,
      [](const QCollator& cmp, const Device& lhs, const Device& rhs) {
        return cmp(lhs.displayName, rhs.displayName);
      },
      Qt::CaseInsensitive, false);*/

  mAbort = false;
  mLoadedFilePath = dir;
  return errors;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void KiCadLibraryImport::open(const FilePath& dir, QStringList& errors) {
  MessageLogger log;

  // Find symbol libraries.
  QDir qDir(dir.toStr());
  /*qDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
  qDir.setNameFilters({"*.kicad_sym"});
  foreach (const QFileInfo& info, qDir.entryInfoList()) {
    const FilePath fp{info.absoluteFilePath()};
    MessageLogger symLog(&log, info.fileName());
    try {
      std::unique_ptr<SExpression> root = SExpression::parse(
          FileUtils::readFile(fp), fp, SExpression::Mode::Permissive);
      KiCadSymbolLibrary lib = KiCadSymbolLibrary::parse(*root, symLog);
      foreach (const auto& symbol, lib.symbols) {
        mSymbols.append(Symbol{
            symbol.name, Qt::Unchecked,
            // std::make_shared<parseagle::Symbol>(symbol),
        });
      }
    } catch (const Exception& e) {
      symLog.critical(e.getMsg());
    }
  }*/

  // Find footprint libraries.
  qDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  qDir.setNameFilters({"*.pretty"});
  foreach (const QFileInfo& dirInfo, qDir.entryInfoList()) {
    QDir qDir(dirInfo.absoluteFilePath());
    qDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    qDir.setNameFilters({"*.kicad_mod"});
    foreach (const QFileInfo& fileInfo, qDir.entryInfoList()) {
      const FilePath fp{fileInfo.absoluteFilePath()};
      MessageLogger fptLog(&log, fileInfo.fileName());
      try {
        std::unique_ptr<SExpression> root = SExpression::parse(
            FileUtils::readFile(fp), fp, SExpression::Mode::Permissive);
        KiCadFootprint fpt = KiCadFootprint::parse(*root, fptLog);
        mPackages.append(Package{
            fpt.name, Qt::Unchecked,
            // std::make_shared<parseagle::Symbol>(symbol),
        });
      } catch (const Exception& e) {
        fptLog.critical(e.getMsg());
      }
    }
  }

  // Find 3D model libraries.
  /*qDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  qDir.setNameFilters({"*.3dshapes"});
  foreach (const QFileInfo& dirInfo, qDir.entryInfoList()) {
    QDir qDir(dirInfo.absoluteFilePath());
    qDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    qDir.setNameFilters({"*.step"});
    foreach (const QFileInfo& fileInfo, qDir.entryInfoList()) {
      const FilePath fp{fileInfo.absoluteFilePath()};
    }
  }*/
}

template <typename T>
int KiCadLibraryImport::getCheckedElementsCount(
    const QVector<T>& elements) const noexcept {
  return std::count_if(elements.begin(), elements.end(), [](const T& element) {
    return element.checkState != Qt::Unchecked;
  });
}

template <typename T>
void KiCadLibraryImport::setElementChecked(QVector<T>& elements,
                                           const QString& name,
                                           bool checked) noexcept {
  bool modified = false;
  Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;
  for (T& element : elements) {
    if ((element.displayName == name) && (element.checkState != checkState)) {
      element.checkState = checkState;
      modified = true;
    }
  }
  if (modified) {
    updateDependencies();
  }
}

void KiCadLibraryImport::updateDependencies() noexcept {
  QSet<QString> dependentPackages;
  QSet<QString> dependentComponents;
  foreach (const Device& dev, mDevices) {
    if (dev.checkState != Qt::Unchecked) {
      dependentComponents.insert(dev.componentDisplayName);
      dependentPackages.insert(dev.packageDisplayName);
    }
  }

  QSet<QString> dependentSymbols;
  for (Component& cmp : mComponents) {
    if (setElementDependent(cmp,
                            dependentComponents.contains(cmp.displayName))) {
      emit componentCheckStateChanged(cmp.displayName, cmp.checkState);
    }
    if (cmp.checkState != Qt::Unchecked) {
      dependentSymbols |= cmp.symbolDisplayNames;
    }
  }

  for (Package& pkg : mPackages) {
    if (setElementDependent(pkg, dependentPackages.contains(pkg.displayName))) {
      emit packageCheckStateChanged(pkg.displayName, pkg.checkState);
    }
  }

  for (Symbol& sym : mSymbols) {
    if (setElementDependent(sym, dependentSymbols.contains(sym.displayName))) {
      emit symbolCheckStateChanged(sym.displayName, sym.checkState);
    }
  }
}

template <typename T>
bool KiCadLibraryImport::setElementDependent(T& element,
                                             bool dependent) noexcept {
  if (dependent && (element.checkState == Qt::Unchecked)) {
    element.checkState = Qt::PartiallyChecked;
    return true;
  } else if ((!dependent) && (element.checkState == Qt::PartiallyChecked)) {
    element.checkState = Qt::Unchecked;
    return true;
  }
  return false;
}

void KiCadLibraryImport::run() noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  std::shared_ptr<MessageLogger> globalLog = mLogger;
  KiCadLibraryConverter converter(*mSettings, this);

  /*int totalCount = getCheckedElementsCount();
  int count = 0;

   foreach (const Symbol& sym, mSymbols) {
     if (mAbort) {
       break;
     }
     if (sym.checkState == Qt::Unchecked) {
       continue;
     }
     MessageLogger log(globalLog.get(), sym.displayName);
     try {
       emit progressStatus(sym.displayName);
       auto symbol = converter.createSymbol(QString(), QString(), *sym.symbol,
                                            log);  // can throw
       TransactionalDirectory dir(TransactionalFileSystem::openRW(
           mDestinationLibraryFp
               .getPathTo(librepcb::Symbol::getShortElementName())
               .getPathTo(symbol->getUuid().toStr())));
       symbol->saveTo(dir);
       dir.getFileSystem()->save();
     } catch (const Exception& e) {
       log.critical(tr("Skipped symbol due to error: %1").arg(e.getMsg()));
     }
     ++count;
     emit progressPercent((100 * count) / std::max(totalCount, 1));
   }

   foreach (const Package& pkg, mPackages) {
     if (mAbort) {
       break;
     }
     if (pkg.checkState == Qt::Unchecked) {
       continue;
     }
     MessageLogger log(globalLog.get(), pkg.displayName);
     try {
       emit progressStatus(pkg.displayName);
       auto package = converter.createPackage(QString(), QString(),
   *pkg.package, log);  // can throw TransactionalDirectory
   dir(TransactionalFileSystem::openRW( mDestinationLibraryFp
               .getPathTo(librepcb::Package::getShortElementName())
               .getPathTo(package->getUuid().toStr())));
       package->saveTo(dir);
       dir.getFileSystem()->save();
     } catch (const Exception& e) {
       log.critical(tr("Skipped package due to error: %1").arg(e.getMsg()));
     }
     ++count;
     emit progressPercent((100 * count) / std::max(totalCount, 1));
   }

   foreach (const Component& cmp, mComponents) {
     if (mAbort) {
       break;
     }
     if (cmp.checkState == Qt::Unchecked) {
       continue;
     }
     MessageLogger log(globalLog.get(), cmp.displayName);
     try {
       emit progressStatus(cmp.displayName);
       auto component =
           converter.createComponent(QString(), QString(), *cmp.deviceSet,
                                     log);  // can throw
       TransactionalDirectory dir(TransactionalFileSystem::openRW(
           mDestinationLibraryFp
               .getPathTo(librepcb::Component::getShortElementName())
               .getPathTo(component->getUuid().toStr())));
       component->saveTo(dir);
       dir.getFileSystem()->save();
     } catch (const Exception& e) {
       log.critical(tr("Skipped component due to error: %1").arg(e.getMsg()));
     }
     ++count;
     emit progressPercent((100 * count) / std::max(totalCount, 1));
   }

   foreach (const Device& dev, mDevices) {
     if (mAbort) {
       break;
     }
     if (dev.checkState == Qt::Unchecked) {
       continue;
     }
     MessageLogger log(globalLog.get(), dev.displayName);
     try {
       emit progressStatus(dev.displayName);
       auto device = converter.createDevice(QString(), QString(),
   *dev.deviceSet, *dev.device, QString(), QString(), log);  // can throw
       TransactionalDirectory dir(TransactionalFileSystem::openRW(
           mDestinationLibraryFp
               .getPathTo(librepcb::Device::getShortElementName())
               .getPathTo(device->getUuid().toStr())));
       device->saveTo(dir);
       dir.getFileSystem()->save();
     } catch (const Exception& e) {
       log.critical(tr("Skipped device due to error: %1").arg(e.getMsg()));
     }
     ++count;
     emit progressPercent((100 * count) / std::max(totalCount, 1));
   }

  emit progressPercent(100);
  emit progressStatus(tr("Finished: %1 of %2 element(s) imported",
                         "Placeholders are numbers", totalCount)
                          .arg(count)
                          .arg(totalCount));
  emit finished();*/
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
