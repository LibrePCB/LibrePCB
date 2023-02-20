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
#include "eaglelibraryimport.h"

#include "eagletypeconverter.cpp"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/tangentpathjoiner.h>
#include <librepcb/core/utils/toolbox.h>
#include <parseagle/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryImport::EagleLibraryImport(const FilePath& dstLibFp,
                                       QObject* parent) noexcept
  : QThread(parent),
    mDestinationLibraryFp(dstLibFp),
    mNamePrefix(),
    mVersion(Version::fromString("0.1")),
    mAuthor("EAGLE Import"),
    mKeywords("eagle,import"),
    mAbort(false) {
}

EagleLibraryImport::~EagleLibraryImport() noexcept {
  // Deleting a QThread while it is running will lead to a crash.
  mAbort = true;
  wait();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int EagleLibraryImport::getTotalElementsCount() const noexcept {
  return mSymbols.count() + mPackages.count() + mComponents.count() +
      mDevices.count();
}

int EagleLibraryImport::getCheckedElementsCount() const noexcept {
  return getCheckedSymbolsCount() + getCheckedPackagesCount() +
      getCheckedComponentsCount() + getCheckedDevicesCount();
}

int EagleLibraryImport::getCheckedSymbolsCount() const noexcept {
  return getCheckedElementsCount(mSymbols);
}

int EagleLibraryImport::getCheckedPackagesCount() const noexcept {
  return getCheckedElementsCount(mPackages);
}

int EagleLibraryImport::getCheckedComponentsCount() const noexcept {
  return getCheckedElementsCount(mComponents);
}

int EagleLibraryImport::getCheckedDevicesCount() const noexcept {
  return getCheckedElementsCount(mDevices);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void EagleLibraryImport::setSymbolChecked(const QString& name,
                                          bool checked) noexcept {
  setElementChecked(mSymbols, name, checked);
}

void EagleLibraryImport::setPackageChecked(const QString& name,
                                           bool checked) noexcept {
  setElementChecked(mPackages, name, checked);
}

void EagleLibraryImport::setComponentChecked(const QString& name,
                                             bool checked) noexcept {
  setElementChecked(mComponents, name, checked);
}

void EagleLibraryImport::setDeviceChecked(const QString& name,
                                          bool checked) noexcept {
  setElementChecked(mDevices, name, checked);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EagleLibraryImport::reset() noexcept {
  mSymbols.clear();
  mPackages.clear();
  mComponents.clear();
  mDevices.clear();
  mLoadedFilePath = FilePath();
}

QStringList EagleLibraryImport::open(const FilePath& lbr) {
  reset();

  QStringList errors;

  try {
    parseagle::Library lib(lbr.toStr(), &errors);

    foreach (const parseagle::Symbol& symbol, lib.getSymbols()) {
      mSymbols.append(Symbol{
          symbol.getName(),
          symbol.getDescription(),
          Qt::Unchecked,
          std::make_shared<parseagle::Symbol>(symbol),
      });
    }

    foreach (const parseagle::Package& package, lib.getPackages()) {
      mPackages.append(Package{
          package.getName(),
          package.getDescription(),
          Qt::Unchecked,
          std::make_shared<parseagle::Package>(package),
      });
    }

    foreach (const parseagle::DeviceSet& deviceSet, lib.getDeviceSets()) {
      QSet<QString> symbolDisplayNames;
      foreach (const parseagle::Gate& gate, deviceSet.getGates()) {
        symbolDisplayNames.insert(gate.getSymbol());
      }
      QString cmpName = deviceSet.getName();
      if ((cmpName.length() > 1) &&
          (cmpName.endsWith("-") || cmpName.endsWith("_"))) {
        cmpName.chop(1);
      }
      mComponents.append(Component{
          cmpName,
          deviceSet.getDescription(),
          Qt::Unchecked,
          symbolDisplayNames,
          std::make_shared<parseagle::DeviceSet>(deviceSet),
      });
      foreach (const parseagle::Device& device, deviceSet.getDevices()) {
        QString name = deviceSet.getName();
        QString suffix = device.getName();
        bool addSeparator = (!name.endsWith("-")) && (!name.endsWith("_")) &&
            (!suffix.startsWith("-")) && (!suffix.startsWith("_"));
        if (addSeparator && (!suffix.isEmpty())) {
          name += "-";
        }
        mDevices.append(Device{
            name + suffix,
            deviceSet.getDescription(),
            Qt::Unchecked,
            cmpName,
            device.getPackage(),
            std::make_shared<parseagle::Device>(device),
            std::make_shared<parseagle::DeviceSet>(deviceSet),
        });
      }
    }

    // Sort all elements by name to improve readability.
    Toolbox::sortNumeric(
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
        Qt::CaseInsensitive, false);

    mAbort = false;
    mLoadedFilePath = lbr;
  } catch (const std::exception& e) {
    qWarning() << "Failed to parse EAGLE library:" << e.what();
    throw RuntimeError(__FILE__, __LINE__, e.what());
  }

  return errors;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename T>
int EagleLibraryImport::getCheckedElementsCount(
    const QVector<T>& elements) const noexcept {
  return std::count_if(elements.begin(), elements.end(), [](const T& element) {
    return element.checkState != Qt::Unchecked;
  });
}

template <typename T>
void EagleLibraryImport::setElementChecked(QVector<T>& elements,
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

void EagleLibraryImport::updateDependencies() noexcept {
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
bool EagleLibraryImport::setElementDependent(T& element,
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

QVector<std::shared_ptr<Polygon> > EagleLibraryImport::convertWires(
    const QString& element, const QList<parseagle::Wire>& wires) {
  QMap<std::pair<GraphicsLayerName, UnsignedLength>,
       QVector<std::shared_ptr<Polygon> > >
      joinablePolygons;
  foreach (const parseagle::Wire& wire, wires) {
    tryOrRaiseError(element, [&joinablePolygons, &wire]() {
      auto polygon = EagleTypeConverter::convertWire(wire);
      auto key =
          std::make_pair(polygon->getLayerName(), polygon->getLineWidth());
      joinablePolygons[key].append(polygon);
    });
  }

  QVector<std::shared_ptr<Polygon> > polygons;
  for (auto it = joinablePolygons.begin(); it != joinablePolygons.end(); it++) {
    QVector<Path> paths;
    foreach (const auto& polygon, it.value()) {
      paths.append(polygon->getPath());
    }
    foreach (const Path& path, TangentPathJoiner::join(paths, 5000)) {
      std::shared_ptr<Polygon> polygon =
          std::make_shared<Polygon>(*it.value().first());
      polygon->setPath(path);
      polygons.append(polygon);
    }
  }
  return polygons;
}

void EagleLibraryImport::tryOrRaiseError(const QString& element,
                                         std::function<void()> func) {
  try {
    func();
  } catch (const Exception& e) {
    raiseImportError(element, e.getMsg());
  }
}

void EagleLibraryImport::raiseImportError(const QString& element,
                                          const QString& error) noexcept {
  QString msg = QString("[%1] ").arg(element) % error;
  mImportErrors.append(msg);
  emit errorOccurred(msg);
}

void EagleLibraryImport::run() noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  mImportErrors.clear();
  int totalCount = getCheckedElementsCount();
  int count = 0;

  QHash<QString, tl::optional<Uuid> > symbolMap;
  QHash<QString, QHash<QString, tl::optional<Uuid> > > symbolPinMap;
  foreach (const Symbol& sym, mSymbols) {
    if (mAbort) {
      break;
    }
    if (sym.checkState == Qt::Unchecked) {
      continue;
    }
    try {
      emit progressStatus(sym.displayName);
      auto symbol = std::make_shared<librepcb::Symbol>(
          Uuid::createRandom(), mVersion, mAuthor,
          EagleTypeConverter::convertElementName(mNamePrefix + sym.displayName),
          EagleTypeConverter::convertElementDescription(sym.description),
          mKeywords);
      symbol->setCategories(mSymbolCategories);
      foreach (const auto& obj,
               convertWires(sym.displayName, sym.symbol->getWires())) {
        if (obj->getPath().isClosed()) {
          obj->setIsGrabArea(true);
        }
        symbol->getPolygons().append(obj);
      }
      foreach (const auto& obj, sym.symbol->getRectangles()) {
        tryOrRaiseError(sym.displayName, [&]() {
          symbol->getPolygons().append(
              EagleTypeConverter::convertRectangle(obj, true));
        });
      }
      foreach (const auto& obj, sym.symbol->getPolygons()) {
        tryOrRaiseError(sym.displayName, [&]() {
          symbol->getPolygons().append(
              EagleTypeConverter::convertPolygon(obj, true));
        });
      }
      foreach (const auto& obj, sym.symbol->getCircles()) {
        tryOrRaiseError(sym.displayName, [&]() {
          symbol->getCircles().append(
              EagleTypeConverter::convertCircle(obj, true));
        });
      }
      foreach (const auto& obj, sym.symbol->getTexts()) {
        tryOrRaiseError(sym.displayName, [&]() {
          symbol->getTexts().append(
              EagleTypeConverter::convertSchematicText(obj));
        });
      }
      foreach (const auto& obj, sym.symbol->getPins()) {
        tryOrRaiseError(sym.displayName, [&]() {
          auto pin = EagleTypeConverter::convertSymbolPin(obj);
          symbol->getPins().append(pin);
          symbolPinMap[sym.symbol->getName()][obj.getName()] = pin->getUuid();
        });
      }
      TransactionalDirectory dir(TransactionalFileSystem::openRW(
          mDestinationLibraryFp
              .getPathTo(librepcb::Symbol::getShortElementName())
              .getPathTo(symbol->getUuid().toStr())));
      symbol->saveTo(dir);
      dir.getFileSystem()->save();
      symbolMap[sym.symbol->getName()] = symbol->getUuid();
    } catch (const Exception& e) {
      raiseImportError(sym.displayName,
                       tr("Skipped symbol due to error: %1").arg(e.getMsg()));
    }
    ++count;
    emit progressPercent((100 * count) / std::max(totalCount, 1));
  }

  QHash<QString, tl::optional<Uuid> > packageMap;
  QHash<QString, QHash<QString, tl::optional<Uuid> > > packagePadMap;
  foreach (const Package& pkg, mPackages) {
    if (mAbort) {
      break;
    }
    if (pkg.checkState == Qt::Unchecked) {
      continue;
    }
    try {
      emit progressStatus(pkg.displayName);
      auto package = std::make_shared<librepcb::Package>(
          Uuid::createRandom(), mVersion, mAuthor,
          EagleTypeConverter::convertElementName(mNamePrefix + pkg.displayName),
          EagleTypeConverter::convertElementDescription(pkg.description),
          mKeywords);
      package->setCategories(mPackageCategories);
      auto footprint = std::make_shared<Footprint>(Uuid::createRandom(),
                                                   ElementName("default"), "");
      package->getFootprints().append(footprint);
      foreach (const auto& obj,
               convertWires(pkg.displayName, pkg.package->getWires())) {
        footprint->getPolygons().append(obj);
      }
      foreach (const auto& obj, pkg.package->getRectangles()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          footprint->getPolygons().append(
              EagleTypeConverter::convertRectangle(obj, false));
        });
      }
      foreach (const auto& obj, pkg.package->getPolygons()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          footprint->getPolygons().append(
              EagleTypeConverter::convertPolygon(obj, false));
        });
      }
      foreach (const auto& obj, pkg.package->getCircles()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          footprint->getCircles().append(
              EagleTypeConverter::convertCircle(obj, false));
        });
      }
      foreach (const auto& obj, pkg.package->getTexts()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          footprint->getStrokeTexts().append(
              EagleTypeConverter::convertBoardText(obj));
        });
      }
      foreach (const auto& obj, pkg.package->getHoles()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          footprint->getHoles().append(EagleTypeConverter::convertHole(obj));
        });
      }
      foreach (const auto& obj, pkg.package->getThtPads()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          auto pair = EagleTypeConverter::convertThtPad(obj);
          package->getPads().append(pair.first);
          footprint->getPads().append(pair.second);
          packagePadMap[pkg.package->getName()][obj.getName()] =
              pair.first->getUuid();
        });
      }
      foreach (const auto& obj, pkg.package->getSmtPads()) {
        tryOrRaiseError(pkg.displayName, [&]() {
          auto pair = EagleTypeConverter::convertSmtPad(obj);
          package->getPads().append(pair.first);
          footprint->getPads().append(pair.second);
          packagePadMap[pkg.package->getName()][obj.getName()] =
              pair.first->getUuid();
        });
      }
      TransactionalDirectory dir(TransactionalFileSystem::openRW(
          mDestinationLibraryFp
              .getPathTo(librepcb::Package::getShortElementName())
              .getPathTo(package->getUuid().toStr())));
      package->saveTo(dir);
      dir.getFileSystem()->save();
      packageMap[pkg.package->getName()] = package->getUuid();
    } catch (const Exception& e) {
      raiseImportError(pkg.displayName,
                       tr("Skipped package due to error: %1").arg(e.getMsg()));
    }
    ++count;
    emit progressPercent((100 * count) / std::max(totalCount, 1));
  }

  QHash<QString, tl::optional<Uuid> > componentMap;
  QHash<QString, QHash<QString, QHash<QString, tl::optional<Uuid> > > >
      componentSignalMap;
  foreach (const Component& cmp, mComponents) {
    if (mAbort) {
      break;
    }
    if (cmp.checkState == Qt::Unchecked) {
      continue;
    }
    try {
      emit progressStatus(cmp.displayName);
      auto component = std::make_shared<librepcb::Component>(
          Uuid::createRandom(), mVersion, mAuthor,
          EagleTypeConverter::convertElementName(mNamePrefix + cmp.displayName),
          EagleTypeConverter::convertElementDescription(cmp.description),
          mKeywords);
      component->setCategories(mComponentCategories);
      component->setPrefixes(NormDependentPrefixMap(
          ComponentPrefix(cmp.deviceSet->getPrefix().trimmed())));
      component->setDefaultValue("{{ PARTNUMBER or DEVICE }}");
      auto symbolVariant = std::make_shared<ComponentSymbolVariant>(
          Uuid::createRandom(), "", ElementName("default"), "");
      component->getSymbolVariants().append(symbolVariant);
      QHash<QString, int> pinCount;
      foreach (const auto& gate, cmp.deviceSet->getGates()) {
        for (auto pinIt = symbolPinMap[gate.getSymbol()].constBegin();
             pinIt != symbolPinMap[gate.getSymbol()].constEnd(); pinIt++) {
          pinCount[pinIt.key()]++;
        }
      }
      foreach (const auto& gate, cmp.deviceSet->getGates()) {
        tl::optional<Uuid> symbolUuid = symbolMap[gate.getSymbol()];
        if (!symbolUuid) {
          throw RuntimeError(__FILE__, __LINE__,
                             tr("Dependent symbol \"%1\" not imported.")
                                 .arg(gate.getSymbol()));
        }
        auto item = std::make_shared<ComponentSymbolVariantItem>(
            Uuid::createRandom(), *symbolUuid,
            EagleTypeConverter::convertPoint(gate.getPosition()), Angle(0),
            true, EagleTypeConverter::convertGateName(gate.getName()));
        symbolVariant->getSymbolItems().append(item);
        for (auto pinIt = symbolPinMap[gate.getSymbol()].constBegin();
             pinIt != symbolPinMap[gate.getSymbol()].constEnd(); pinIt++) {
          Uuid signalUuid = Uuid::createRandom();
          QString signalName = pinIt.key();
          if ((pinCount[signalName] > 1) ||
              (component->getSignals().contains(signalName))) {
            // Name conflict -> add prefix to ensure unique signal names.
            signalName.prepend(*item->getSuffix() % "_");
          }
          component->getSignals().append(std::make_shared<ComponentSignal>(
              signalUuid, EagleTypeConverter::convertPinOrPadName(signalName),
              SignalRole::passive(), QString(), false, false, false));
          item->getPinSignalMap().append(
              std::make_shared<ComponentPinSignalMapItem>(
                  pinIt->value(), signalUuid,
                  CmpSigPinDisplayType::componentSignal()));
          componentSignalMap[cmp.deviceSet->getName()][gate.getName()]
                            [pinIt.key()] = signalUuid;
        }
      }
      TransactionalDirectory dir(TransactionalFileSystem::openRW(
          mDestinationLibraryFp
              .getPathTo(librepcb::Component::getShortElementName())
              .getPathTo(component->getUuid().toStr())));
      component->saveTo(dir);
      dir.getFileSystem()->save();
      componentMap[cmp.deviceSet->getName()] = component->getUuid();
    } catch (const Exception& e) {
      raiseImportError(
          cmp.displayName,
          tr("Skipped component due to error: %1").arg(e.getMsg()));
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
    try {
      emit progressStatus(dev.displayName);
      tl::optional<Uuid> componentUuid = componentMap[dev.deviceSet->getName()];
      if (!componentUuid) {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Dependent component \"%1\" not imported.")
                               .arg(dev.componentDisplayName));
      }
      tl::optional<Uuid> packageUuid = packageMap[dev.device->getPackage()];
      if (!packageUuid) {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Dependent package \"%1\" not imported.")
                               .arg(dev.packageDisplayName));
      }
      std::unique_ptr<librepcb::Device> device(new librepcb::Device(
          Uuid::createRandom(), mVersion, mAuthor,
          EagleTypeConverter::convertElementName(mNamePrefix + dev.displayName),
          EagleTypeConverter::convertElementDescription(dev.description),
          mKeywords, *componentUuid, *packageUuid));
      device->setCategories(mDeviceCategories);
      for (auto padIt = packagePadMap[dev.device->getPackage()].constBegin();
           padIt != packagePadMap[dev.device->getPackage()].constEnd();
           padIt++) {
        tl::optional<Uuid> signalUuid;
        foreach (const auto& connection, dev.device->getConnections()) {
          if (connection.getPads().contains(padIt.key())) {
            signalUuid =
                componentSignalMap[dev.deviceSet->getName()]
                                  [connection.getGate()][connection.getPin()];
          }
        }
        device->getPadSignalMap().append(
            std::make_shared<DevicePadSignalMapItem>(padIt->value(),
                                                     signalUuid));
      }
      TransactionalDirectory dir(TransactionalFileSystem::openRW(
          mDestinationLibraryFp
              .getPathTo(librepcb::Device::getShortElementName())
              .getPathTo(device->getUuid().toStr())));
      device->saveTo(dir);
      dir.getFileSystem()->save();
    } catch (const Exception& e) {
      raiseImportError(dev.displayName,
                       tr("Skipped device due to error: %1").arg(e.getMsg()));
    }
    ++count;
    emit progressPercent((100 * count) / std::max(totalCount, 1));
  }

  emit progressPercent(100);
  emit progressStatus(tr("Finished: %1 of %2 element(s) imported",
                         "Placeholders are numbers", totalCount)
                          .arg(count)
                          .arg(totalCount));
  emit finished(mImportErrors);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb
