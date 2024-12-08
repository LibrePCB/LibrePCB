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
#include "kicadlibraryconverter.h"

#include "kicadtypeconverter.h"
#include "kicadtypes.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/tangentpathjoiner.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace kicadimport {

using C = KiCadTypeConverter;

/*******************************************************************************
 *  Struct KiCadLibraryConverterSettings
 ******************************************************************************/

KiCadLibraryConverterSettings::KiCadLibraryConverterSettings() noexcept
  : namePrefix(),
    version(Version::fromString("0.1")),
    author("KiCad Import"),
    keywords("kicad,import") {
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryConverter::KiCadLibraryConverter(
    WorkspaceLibraryDb& db, const KiCadLibraryConverterSettings& settings,
    QObject* parent) noexcept
  : QObject(parent), mLibraryDb(db), mSettings(settings) {
}

KiCadLibraryConverter::~KiCadLibraryConverter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryConverter::reset() noexcept {
}

std::unique_ptr<Package> KiCadLibraryConverter::createPackage(
    const FilePath& libFp, const KiCadFootprint& kiFpt,
    const QString& generatedBy, const QMap<QString, FilePath>& models,
    MessageLogger& log) {
  if (mPackageMap.contains(generatedBy)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  if (kiFpt.layer != KiCadLayer::FrontCopper) {
    throw RuntimeError(__FILE__, __LINE__, "Unsupported footprint board side.");
  }
  std::unique_ptr<Package> package(new Package(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertElementName(mSettings.namePrefix + kiFpt.name),
      C::convertElementDescription(libFp, kiFpt.name, kiFpt.properties),
      C::convertElementKeywords(mSettings.keywords, kiFpt.properties),
      librepcb::Package::AssemblyType::Auto));
  package->setGeneratedBy(generatedBy);
  package->setCategories(mSettings.packageCategories);
  package->setResources(C::convertResources(kiFpt.properties));

  // Assembly type.
  const bool exclude = kiFpt.excludeFromBom || kiFpt.excludeFromPosFiles;
  if ((kiFpt.isSmd) && (!kiFpt.isThroughHole) && (!exclude)) {
    package->setAssemblyType(Package::AssemblyType::Smt);
  } else if ((!kiFpt.isSmd) && (kiFpt.isThroughHole) && (!exclude)) {
    package->setAssemblyType(Package::AssemblyType::Tht);
  } else if ((kiFpt.isSmd) && (kiFpt.isThroughHole) && (!exclude)) {
    package->setAssemblyType(Package::AssemblyType::Mixed);
  } else if (exclude) {
    package->setAssemblyType(Package::AssemblyType::None);
  }

  // Footprint.
  auto footprint = std::make_shared<Footprint>(Uuid::createRandom(),
                                               ElementName("default"), "");
  package->getFootprints().append(footprint);

  // Geometry.
  QList<C::Line> lines;
  for (const KiCadFootprintLine& line : kiFpt.lines) {
    tryOrLogError([&]() { lines.append(C::convertFootprintLine(line)); }, log);
  }
  for (const KiCadFootprintArc& arc : kiFpt.arcs) {
    tryOrLogError([&]() { lines.append(C::convertFootprintArc(arc)); }, log);
  }
  const QList<C::LineGroup> lineGroups = C::groupLinesByLayerAndWidth(lines);
  bool timedOut = false;
  for (const C::LineGroup& group : lineGroups) {
    tryOrLogError(
        [&]() {
          const qint64 timeout =
              generatedBy.contains("TerminalBlock_WAGO") ? 500 : 5000;
          foreach (const Path& path,
                   TangentPathJoiner::join(group.paths, timeout, &timedOut)) {
            footprint->getPolygons().append(
                std::make_shared<Polygon>(Uuid::createRandom(), *group.layer,
                                          group.width, false, false, path));
          }
        },
        log);
  }
  if (timedOut) {
    log.info(
        "Aborted joining tangent line segments to polygons due to timeout, "
        "keeping them separate.");
  }
  for (const KiCadFootprintCircle& circle : kiFpt.circles) {
    tryOrLogError(
        [&]() {
          footprint->getCircles().append(C::convertFootprintCircle(circle));
        },
        log);
  }
  for (const KiCadFootprintRectangle& rect : kiFpt.rectangles) {
    tryOrLogError(
        [&]() {
          footprint->getPolygons().append(C::convertFootprintRectangle(rect));
        },
        log);
  }
  for (const KiCadFootprintPolygon& polygon : kiFpt.polygons) {
    tryOrLogError(
        [&]() {
          footprint->getPolygons().append(C::convertFootprintPolygon(polygon));
        },
        log);
  }
  for (const KiCadProperty& property : kiFpt.properties) {
    tryOrLogError(
        [&]() {
          if (auto st = C::convertFootprintPropertyToText(property)) {
            footprint->getStrokeTexts().append(st);
          }
        },
        log);
  }
  for (const KiCadFootprintText& kiText : kiFpt.texts) {
    tryOrLogError(
        [&]() {
          if (auto st = C::convertFootprintText(kiText)) {
            footprint->getStrokeTexts().append(st);
          }
        },
        log);
  }

  // Zones.
  for (const KiCadZone& kiZone : kiFpt.zones) {
    tryOrLogError(
        [&]() {
          if (auto zone = C::convertFootprintZone(kiZone, log)) {
            footprint->getZones().append(zone);
          }
        },
        log);
  }

  // Pads.
  for (const KiCadFootprintPad& obj : kiFpt.pads) {
    tryOrLogError(
        [&]() {
          C::Pad res = C::convertPad(
              obj, kiFpt.solderMaskMargin, kiFpt.solderPasteMargin,
              kiFpt.solderPasteRatio, kiFpt.clearance, log);
          if (res.fptPad) {
            std::shared_ptr<PackagePad> pkgPad =
                package->getPads().find(obj.number);
            if ((!pkgPad) && (!obj.number.isEmpty())) {
              pkgPad = std::make_shared<PackagePad>(
                  res.fptPad->getUuid(), CircuitIdentifier(obj.number));
              package->getPads().append(pkgPad);
              mPackagePadMap[generatedBy][*pkgPad->getName()] =
                  pkgPad->getUuid();
            }
            if (pkgPad) {
              res.fptPad->setPackagePadUuid(pkgPad->getUuid());
            }
            footprint->getPads().append(res.fptPad);
          }
          if (res.hole) {
            footprint->getHoles().append(res.hole);
          }
          for (auto polygon : res.polygons) {
            footprint->getPolygons().append(polygon);
          }
        },
        log);
  }

  // 3D models.
  for (const KiCadFootprintModel& kiModel : kiFpt.models) {
    tryOrLogError(
        [&]() {
          const FilePath fp = models.value(kiModel.path);
          if (fp.isValid()) {
            const Uuid uuid = Uuid::createRandom();
            auto model = std::make_shared<PackageModel>(
                uuid, ElementName(cleanElementName(fp.getCompleteBasename())));
            package->getDirectory().write(model->getFileName(),
                                          FileUtils::readFile(fp));
            package->getModels().append(model);
            footprint->setModelPosition(
                std::make_tuple(Length::fromMm(kiModel.offset.x()),
                                Length::fromMm(kiModel.offset.y()),
                                Length::fromMm(kiModel.offset.z())));
            footprint->setModelRotation(
                std::make_tuple(Angle::fromDeg(kiModel.rotate.x()),
                                Angle::fromDeg(kiModel.rotate.y()),
                                -Angle::fromDeg(kiModel.rotate.z())));
            if (kiModel.scale == QVector3D(1, 1, 1)) {
              footprint->setModels(QSet<Uuid>{uuid});
            } else {
              log.warning(
                  "Scale factor on 3D model is not supported, will be "
                  "ignored.");
            }
          }
        },
        log);
  }

  mPackageMap[generatedBy] = package->getUuid();
  return package;
}

std::unique_ptr<Symbol> KiCadLibraryConverter::createSymbol(
    const FilePath& libFp, const KiCadSymbol& kiSym,
    const KiCadSymbolGate& kiGate, const QString& generatedBy,
    MessageLogger& log) {
  if (mSymbolMap.contains(generatedBy)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Symbol> symbol(new Symbol(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertElementName(mSettings.namePrefix + kiGate.name),
      C::convertElementDescription(libFp, kiGate.name, kiSym.properties),
      C::convertElementKeywords(mSettings.keywords, kiSym.properties)));
  symbol->setGeneratedBy(generatedBy);
  symbol->setCategories(mSettings.symbolCategories);
  symbol->setResources(C::convertResources(kiSym.properties));

  // Geometries.
  for (const KiCadSymbolArc& arc : kiGate.arcs) {
    tryOrLogError(
        [&]() { symbol->getPolygons().append(C::convertSymbolArc(arc)); }, log);
  }
  for (const KiCadSymbolCircle& circle : kiGate.circles) {
    tryOrLogError(
        [&]() { symbol->getCircles().append(C::convertSymbolCircle(circle)); },
        log);
  }
  for (const KiCadSymbolRectangle& rect : kiGate.rectangles) {
    tryOrLogError(
        [&]() {
          symbol->getPolygons().append(C::convertSymbolRectangle(rect));
        },
        log);
  }
  for (const KiCadSymbolPolyline& polyline : kiGate.polylines) {
    tryOrLogError(
        [&]() {
          symbol->getPolygons().append(C::convertSymbolPolyline(polyline));
        },
        log);
  }
  for (const KiCadSymbolText& kiText : kiGate.texts) {
    tryOrLogError(
        [&]() {
          if (auto t = C::convertSymbolText(kiText)) {
            symbol->getTexts().append(t);
          }
        },
        log);
  }
  for (const KiCadProperty& property : kiSym.properties) {
    tryOrLogError(
        [&]() {
          if (auto t = C::convertSymbolPropertyToText(property)) {
            symbol->getTexts().append(t);
          }
        },
        log);
  }

  // Pins.
  const QList<std::pair<QString, QStringList>> pinNames =
      C::convertSymbolPinNames(kiGate.pins);
  for (int i = 0; i < kiGate.pins.count(); ++i) {
    tryOrLogError(
        [&]() {
          const QString pinName = pinNames.value(i).first;
          if (pinName.isEmpty()) return;
          const auto pin = C::convertSymbolPin(kiGate.pins.value(i), pinName,
                                               kiSym.pinNamesOffset);
          symbol->getPins().append(pin);
          mSymbolPinMap[std::make_pair(generatedBy, pinName)] = pin->getUuid();
        },
        log);
  }

  mSymbolMap[generatedBy] = symbol->getUuid();
  return symbol;
}

std::unique_ptr<Component> KiCadLibraryConverter::createComponent(
    const FilePath& libFp, const KiCadSymbol& kiSym,
    const QList<KiCadSymbolGate>& kiGates, const QString& generatedBy,
    const QStringList& symGeneratedBy, MessageLogger& log) {
  Q_UNUSED(log);
  if (kiGates.count() != symGeneratedBy.count()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mComponentMap.contains(generatedBy)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  for (const QString& symGenBy : symGeneratedBy) {
    if (!mSymbolMap.contains(symGenBy)) {
      loadAlreadyImportedSymbol(symGenBy);  // can throw
    }
  }
  std::unique_ptr<Component> component(new Component(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertElementName(mSettings.namePrefix + kiSym.name),
      C::convertElementDescription(libFp, kiSym.name, kiSym.properties),
      C::convertElementKeywords(mSettings.keywords, kiSym.properties)));
  component->setGeneratedBy(generatedBy);
  component->setCategories(mSettings.componentCategories);
  component->setResources(C::convertResources(kiSym.properties));
  component->setIsSchematicOnly(!kiSym.onBoard);
  if (auto p = C::findProperty(kiSym.properties, "reference")) {
    component->setPrefixes(NormDependentPrefixMap(
        ComponentPrefix(cleanComponentPrefix(p->value))));
  }
  component->setDefaultValue("{{ MPN or DEVICE }}");
  auto symbolVariant = std::make_shared<ComponentSymbolVariant>(
      Uuid::createRandom(), "", ElementName("default"), "");
  component->getSymbolVariants().append(symbolVariant);

  const QMap<KiCadPinType, const SignalRole*> signalRoleMap = {
      {KiCadPinType::Input, &SignalRole::input()},
      {KiCadPinType::Output, &SignalRole::output()},
      {KiCadPinType::Bidirectional, &SignalRole::inout()},
      {KiCadPinType::Passive, &SignalRole::passive()},
      {KiCadPinType::PowerIn, &SignalRole::power()},
      {KiCadPinType::PowerOut, &SignalRole::power()},
      {KiCadPinType::OpenCollector, &SignalRole::opendrain()},
  };

  const bool addGateSuffixes = kiGates.count() > 1;
  const QString suffixes = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int i = 0; i < kiGates.count(); ++i) {
    const QString symGenBy = symGeneratedBy.value(i);
    const std::optional<Uuid> symbolUuid = mSymbolMap.value(symGenBy);
    if (!symbolUuid) {
      throw LogicError(__FILE__, __LINE__);
    }
    const KiCadSymbolGate& gate = kiGates.at(i);
    auto item = std::make_shared<ComponentSymbolVariantItem>(
        Uuid::createRandom(), *symbolUuid, Point(0, 0), Angle(0), true,
        ComponentSymbolVariantItemSuffix(addGateSuffixes ? suffixes.mid(i, 1)
                                                         : ""));
    symbolVariant->getSymbolItems().append(item);

    const QList<std::pair<QString, QStringList>> pinNames =
        C::convertSymbolPinNames(gate.pins);
    for (int i = 0; i < gate.pins.count(); ++i) {
      const QString pinName = pinNames.value(i).first;
      if (pinName.isEmpty()) continue;
      const KiCadSymbolPin& pin = gate.pins.at(i);
      Uuid signalUuid = Uuid::createRandom();
      const SignalRole& signalRole =
          *signalRoleMap.value(pin.type, &SignalRole::passive());
      const CmpSigPinDisplayType& displayType =
          CmpSigPinDisplayType::componentSignal();
      const bool isRequired = false;
      const bool isNegated = (pin.shape == KiCadPinStyle::Inverted) ||
          (pin.shape == KiCadPinStyle::InvertedClock);
      const bool isClock = (pin.shape == KiCadPinStyle::Clock) ||
          (pin.shape == KiCadPinStyle::ClockLow) ||
          (pin.shape == KiCadPinStyle::EdgeClockHigh) ||
          (pin.shape == KiCadPinStyle::InvertedClock);
      component->getSignals().append(std::make_shared<ComponentSignal>(
          signalUuid, CircuitIdentifier(pinName), signalRole, QString(),
          isRequired, isNegated, isClock));

      std::optional<Uuid> pinUuid =
          mSymbolPinMap.value(std::make_pair(symGenBy, pinName));
      if (!pinUuid) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Pin '%1' not found in symbol.").arg(pinName));
      }
      item->getPinSignalMap().append(
          std::make_shared<ComponentPinSignalMapItem>(*pinUuid, signalUuid,
                                                      displayType));
      mComponentSignalMap.insert(std::make_pair(generatedBy, pinName),
                                 signalUuid);
    }
  }
  mComponentMap[generatedBy] = component->getUuid();
  return component;
}

std::unique_ptr<Device> KiCadLibraryConverter::createDevice(
    const FilePath& libFp, const KiCadSymbol& kiSym,
    const QList<KiCadSymbolGate>& kiGates, const QString& generatedBy,
    const QString& cmpGeneratedBy, const QString& pkgGeneratedBy,
    MessageLogger& log) {
  Q_UNUSED(log);
  if (!mComponentMap.contains(cmpGeneratedBy)) {
    loadAlreadyImportedComponent(cmpGeneratedBy);  // can throw
  }
  const std::optional<Uuid> componentUuid = mComponentMap.value(cmpGeneratedBy);
  if (!componentUuid) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (!mPackageMap.contains(pkgGeneratedBy)) {
    loadAlreadyImportedPackage(pkgGeneratedBy);  // can throw
  }
  const std::optional<Uuid> packageUuid = mPackageMap.value(pkgGeneratedBy);
  if (!packageUuid) {
    throw LogicError(__FILE__, __LINE__);
  }
  std::unique_ptr<Device> device(new Device(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertElementName(mSettings.namePrefix + kiSym.name),
      C::convertElementDescription(libFp, kiSym.name, kiSym.properties),
      C::convertElementKeywords(mSettings.keywords, kiSym.properties),
      *componentUuid, *packageUuid));
  device->setGeneratedBy(generatedBy);
  device->setCategories(mSettings.deviceCategories);
  device->setResources(C::convertResources(kiSym.properties));

  QSet<QString> connectedPads;
  for (auto it = mPackagePadMap[pkgGeneratedBy].constBegin();
       it != mPackagePadMap[pkgGeneratedBy].constEnd(); it++) {
    std::optional<Uuid> signalUuid;
    for (const KiCadSymbolGate& gate : kiGates) {
      const QList<std::pair<QString, QStringList>> pinNames =
          C::convertSymbolPinNames(gate.pins);
      for (int i = 0; i < gate.pins.count(); ++i) {
        if (pinNames.value(i).second.contains(it.key())) {
          signalUuid = mComponentSignalMap.value(
              std::make_pair(cmpGeneratedBy, pinNames.value(i).first));
          connectedPads.insert(it.key());
        }
      }
    }
    device->getPadSignalMap().append(
        std::make_shared<DevicePadSignalMapItem>(it->value(), signalUuid));
  }

  // Fail if the symbol specifies pad numbers we didn't find in the package.
  for (const KiCadSymbolGate& gate : kiGates) {
    const QList<std::pair<QString, QStringList>> pinNames =
        C::convertSymbolPinNames(gate.pins);
    for (const auto& pair : pinNames) {
      for (const QString& padNumber : pair.second) {
        if (!connectedPads.contains(padNumber)) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Pad '%1' not found in imported package.")
                                 .arg(padNumber));
        }
      }
    }
  }

  return device;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void KiCadLibraryConverter::loadAlreadyImportedSymbol(
    const QString& generatedBy) {
  const FilePath fp = getAlreadyImportedFp<Symbol>(generatedBy);  // can throw
  std::unique_ptr<Symbol> symbol = Symbol::open(
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
          std::make_shared<TransactionalFileSystem>(fp))));  // can throw
  for (const SymbolPin& pin : symbol->getPins()) {
    mSymbolPinMap.insert(std::make_pair(generatedBy, *pin.getName()),
                         pin.getUuid());
  }
  mSymbolMap[generatedBy] = symbol->getUuid();
}

void KiCadLibraryConverter::loadAlreadyImportedPackage(
    const QString& generatedBy) {
  const FilePath fp = getAlreadyImportedFp<Package>(generatedBy);  // can throw
  std::unique_ptr<Package> package = Package::open(
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
          std::make_shared<TransactionalFileSystem>(fp))));  // can throw
  for (const PackagePad& pad : package->getPads()) {
    mPackagePadMap[generatedBy][*pad.getName()] = pad.getUuid();
  }
  mPackageMap[generatedBy] = package->getUuid();
}

void KiCadLibraryConverter::loadAlreadyImportedComponent(
    const QString& generatedBy) {
  const FilePath fp =
      getAlreadyImportedFp<Component>(generatedBy);  // can throw
  std::unique_ptr<Component> component = Component::open(
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
          std::make_shared<TransactionalFileSystem>(fp))));  // can throw
  for (const ComponentSignal& signal : component->getSignals()) {
    mComponentSignalMap.insert(std::make_pair(generatedBy, *signal.getName()),
                               signal.getUuid());
  }
  mComponentMap[generatedBy] = component->getUuid();
}

template <typename T>
FilePath KiCadLibraryConverter::getAlreadyImportedFp(
    const QString& generatedBy) const {
  QSet<Uuid> uuids = mLibraryDb.getGenerated<T>(generatedBy);
  for (const Uuid& uuid : uuids) {
    const FilePath fp = mLibraryDb.getLatest<T>(uuid);
    if (fp.isValid()) return fp;
  }
  const QString ref =
      QString(generatedBy).replace("KiCadImport::", "").replace("::", ":");
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Dependent %1 '%2' not found.")
                         .arg(T::getLongElementName())
                         .arg(ref));
}

void KiCadLibraryConverter::tryOrLogError(std::function<void()> func,
                                          MessageLogger& log) {
  try {
    func();
  } catch (const Exception& e) {
    log.critical(e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
