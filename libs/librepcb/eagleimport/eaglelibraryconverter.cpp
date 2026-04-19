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
#include "eaglelibraryconverter.h"

#include "eagletypeconverter.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/messagelogger.h>
#include <parseagle/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {

using C = EagleTypeConverter;

static QString generatedBy(QString libName, QStringList keys) {
  if (libName.toLower().endsWith(".lbr")) {
    libName.chop(4);
  }
  keys.prepend(libName);
  keys.prepend("EagleImport");
  return keys.join("::");
}

/*******************************************************************************
 *  Struct EagleLibraryConverterSettings
 ******************************************************************************/

EagleLibraryConverterSettings::EagleLibraryConverterSettings() noexcept
  : createUuid(&Uuid::createRandom),
    namePrefix(),
    version(Version::fromString("0.1")),
    author("EAGLE Import"),
    created(QDateTime::currentDateTime()),
    keywords("eagle,import"),
    autoThtAnnularWidth(EagleTypeConverter::getDefaultAutoThtAnnularWidth()) {
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryConverter::EagleLibraryConverter(
    const EagleLibraryConverterSettings& settings, QObject* parent) noexcept
  : QObject(parent),
    mSettings(settings),
    mTc(new EagleTypeConverter(settings.createUuid)) {
}

EagleLibraryConverter::~EagleLibraryConverter() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Uuid EagleLibraryConverter::getComponentSignalOfSymbolPin(
    const QString& libName, const QString& libUrn, const QString& devSetName,
    const QString& gateName, const QString& pinName) const {
  const QStringList key = {libName, libUrn, devSetName, gateName, pinName};
  if (auto uuid = mComponentSignalMap.value(key)) {
    return *uuid;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Could not find component signal from pin name: %1")
            .arg(pinName));
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EagleLibraryConverter::reset() noexcept {
  mSymbolMap.clear();
  mSymbolPinMap.clear();
  mPackageMap.clear();
  mPackagePadMap.clear();
  mComponentMap.clear();
  mComponentSignalMap.clear();
}

std::unique_ptr<Symbol> EagleLibraryConverter::createSymbol(
    const QString& libName, const QString& libUrn,
    const parseagle::Symbol& eagleSymbol, MessageLogger& log) {
  const QStringList key = {libName, libUrn, eagleSymbol.getName()};
  if (mSymbolMap.contains(key)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Symbol> symbol(new Symbol(
      mSettings.createUuid(), mSettings.version, mSettings.author,
      mSettings.created,
      mTc->convertElementName(mSettings.namePrefix + eagleSymbol.getName()),
      mTc->convertElementDescription(eagleSymbol.getDescription()),
      mSettings.keywords));
  symbol->setGeneratedBy(generatedBy(libName, {eagleSymbol.getName()}));
  symbol->setCategories(mSettings.symbolCategories);

  QList<C::Geometry> geometries;
  tryOrLogError(
      [&]() {
        // Enable grab areas on closed polygons. However, don't do this for
        // sheet frames as it would look ugly. We guess that by the absence of
        // pins.
        const bool grabArea = !eagleSymbol.getPins().isEmpty();
        geometries +=
            mTc->convertAndJoinWires(eagleSymbol.getWires(), grabArea, log);
      },
      log);
  foreach (const auto& obj, eagleSymbol.getRectangles()) {
    geometries.append(mTc->convertRectangle(obj, true));
  }
  foreach (const auto& obj, eagleSymbol.getPolygons()) {
    geometries.append(mTc->convertPolygon(obj, true));
  }
  foreach (const auto& obj, eagleSymbol.getCircles()) {
    geometries.append(mTc->convertCircle(obj, true));
  }
  foreach (const auto& obj, eagleSymbol.getFrames()) {
    geometries.append(mTc->convertFrame(obj));
  }
  // Disable grab area on geometries located *within* another grab area to
  // avoid overlapping grab areas, but also to avoid triggering issue
  // https://github.com/LibrePCB/LibrePCB/issues/1278.
  std::stable_sort(geometries.begin(), geometries.end(),
            [](const EagleTypeConverter::Geometry& a,
               const EagleTypeConverter::Geometry& b) {
              if (a.path.isClosed() != b.path.isClosed()) {
                return a.path.isClosed();
              }
              if (a.path.isClosed()) {
                // Use combined absolute+relative tolerance to avoid sub-ULP
                // floating-point differences (accumulation order, FMA) from
                // producing platform-dependent orderings. The absolute floor
                // (1e-6 mm²) prevents degenerate near-zero areas (e.g. all
                // vertices collinear) from comparing unequal across platforms.
                const double a1 = a.path.calcAreaOfStraightSegments();
                const double a2 = b.path.calcAreaOfStraightSegments();
                const double maxArea = std::max(a1, a2);
                if ((maxArea > 1e-6) &&
                    (std::abs(a1 - a2) > (maxArea * 1e-12))) {
                  return a1 > a2;
                }
              }
              const UnsignedLength l1 = a.path.getTotalStraightLength();
              const UnsignedLength l2 = b.path.getTotalStraightLength();
              if ((*l1 - *l2).abs() < Length(50)) {
                return l1 > l2;
              }
              return false;
            });
  QPainterPath totalGrabArea;
  totalGrabArea.setFillRule(Qt::WindingFill);
  for (EagleTypeConverter::Geometry& g : geometries) {
    if (g.grabArea) {
      const QPainterPath p = g.path.toQPainterPathPx();
      if (totalGrabArea.contains(p)) {
        g.grabArea = false;
      } else {
        totalGrabArea |= p;
      }
    }
  }
  foreach (const auto& g, geometries) {
    tryOrLogError(
        [&]() {
          if (auto o = mTc->tryConvertToSchematicCircle(g)) {
            symbol->getCircles().append(o);
          } else if (auto o = mTc->tryConvertToSchematicPolygon(g)) {
            symbol->getPolygons().append(o);
          } else {
            log.warning(tr("Skipped graphics object on layer %1 (%2).")
                            .arg(g.layerId)
                            .arg(C::getLayerName(g.layerId)));
          }
        },
        log);
  }
  foreach (const auto& obj, eagleSymbol.getTexts()) {
    if (auto lpObj = mTc->tryConvertSchematicText(obj, true)) {
      symbol->getTexts().append(lpObj);
    } else {
      log.warning(tr("Skipped text on layer %1 (%2).")
                      .arg(obj.getLayer())
                      .arg(C::getLayerName(obj.getLayer())));
    }
  }
  foreach (const auto& obj, eagleSymbol.getPins()) {
    tryOrLogError(
        [&]() {
          const auto pinObj = mTc->convertSymbolPin(obj);
          symbol->getPins().append(pinObj.pin);
          mSymbolPinMap[key][obj.getName()] = std::make_pair(
              std::make_shared<parseagle::Pin>(obj), pinObj.pin->getUuid());
          if (pinObj.circle) {
            symbol->getCircles().append(pinObj.circle);
          }
          if (pinObj.polygon) {
            symbol->getPolygons().append(pinObj.polygon);
          }
        },
        log);
  }
  mSymbolMap[key] = symbol->getUuid();
  return symbol;
}

std::unique_ptr<Package> EagleLibraryConverter::createPackage(
    const QString& libName, const QString& libUrn,
    const parseagle::Package& eaglePackage, MessageLogger& log) {
  const QStringList key = {libName, libUrn, eaglePackage.getName()};
  if (mPackageMap.contains(key)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Package> package(new Package(
      mSettings.createUuid(), mSettings.version, mSettings.author,
      mSettings.created,
      mTc->convertElementName(mSettings.namePrefix + eaglePackage.getName()),
      mTc->convertElementDescription(eaglePackage.getDescription()),
      mSettings.keywords, librepcb::Package::AssemblyType::Auto));
  package->setGeneratedBy(generatedBy(libName, {eaglePackage.getName()}));
  package->setCategories(mSettings.packageCategories);
  auto footprint = std::make_shared<Footprint>(mSettings.createUuid(),
                                               ElementName("default"), "");
  package->getFootprints().append(footprint);

  QList<C::Geometry> geometries;
  tryOrLogError(
      [&]() {
        geometries +=
            mTc->convertAndJoinWires(eaglePackage.getWires(), false, log);
      },
      log);
  foreach (const auto& obj, eaglePackage.getRectangles()) {
    geometries.append(mTc->convertRectangle(obj, false));
  }
  foreach (const auto& obj, eaglePackage.getPolygons()) {
    geometries.append(mTc->convertPolygon(obj, false));
  }
  foreach (const auto& obj, eaglePackage.getCircles()) {
    geometries.append(mTc->convertCircle(obj, false));
  }
  foreach (const auto& g, geometries) {
    tryOrLogError(
        [&]() {
          const auto zones = mTc->tryConvertToBoardZones(g);
          if (!zones.isEmpty()) {
            foreach (const auto& o, zones) {
              footprint->getZones().append(o);
            }
          } else if (auto o = mTc->tryConvertToBoardCircle(g)) {
            footprint->getCircles().append(o);
          } else if (auto o = mTc->tryConvertToBoardPolygon(g)) {
            footprint->getPolygons().append(o);
          } else {
            log.warning(tr("Skipped graphics object on layer %1 (%2).")
                            .arg(g.layerId)
                            .arg(C::getLayerName(g.layerId)));
          }
        },
        log);
  }
  foreach (const auto& obj, eaglePackage.getTexts()) {
    if (auto lpObj = mTc->tryConvertBoardText(obj, true)) {
      footprint->getStrokeTexts().append(lpObj);
    } else {
      log.warning(tr("Skipped text on layer %1 (%2).")
                      .arg(obj.getLayer())
                      .arg(C::getLayerName(obj.getLayer())));
    }
  }
  foreach (const auto& obj, eaglePackage.getHoles()) {
    tryOrLogError(
        [&]() { footprint->getHoles().append(mTc->convertHole(obj)); }, log);
  }
  foreach (const auto& obj, eaglePackage.getThtPads()) {
    tryOrLogError(
        [&]() {
          auto pair = mTc->convertThtPad(obj, mSettings.autoThtAnnularWidth);
          package->getPads().append(pair.first);
          footprint->getPads().append(pair.second);
          mPackagePadMap[key][obj.getName()] = pair.first->getUuid();
        },
        log);
  }
  foreach (const auto& obj, eaglePackage.getSmtPads()) {
    tryOrLogError(
        [&]() {
          auto pair = mTc->convertSmtPad(obj);
          package->getPads().append(pair.first);
          footprint->getPads().append(pair.second);
          mPackagePadMap[key][obj.getName()] = pair.first->getUuid();
        },
        log);
  }
  // If there is exactly one device keepout zone on the top layer, convert
  // it to a package courtyard polygon.
  auto courtyardZone = footprint->getZones().value(0);
  if ((footprint->getZones().count() == 1) &&
      (courtyardZone->getLayers() == Zone::Layers(Zone::Layer::Top)) &&
      (courtyardZone->getRules() == Zone::Rules(Zone::Rule::NoDevices))) {
    footprint->getPolygons().append(std::make_shared<Polygon>(
        mSettings.createUuid(), Layer::topCourtyard(), UnsignedLength(0), false,
        false, courtyardZone->getOutline()));
    footprint->getZones().clear();
  }
  mPackageMap[key] = package->getUuid();
  return package;
}

std::unique_ptr<Component> EagleLibraryConverter::createComponent(
    const QString& libName, const QString& libUrn,
    const parseagle::DeviceSet& eagleDeviceSet, MessageLogger& log) {
  Q_UNUSED(log);
  const QMap<parseagle::PinVisibility, const CmpSigPinDisplayType*>
      displayTypeMap = {
          {parseagle::PinVisibility::Off, &CmpSigPinDisplayType::none()},
          {parseagle::PinVisibility::Pad, &CmpSigPinDisplayType::none()},
          {parseagle::PinVisibility::Pin,
           &CmpSigPinDisplayType::componentSignal()},
          {parseagle::PinVisibility::Both,
           &CmpSigPinDisplayType::componentSignal()},
      };
  const QMap<parseagle::PinDirection, const SignalRole*> signalRoleMap = {
      {parseagle::PinDirection::NotConnected, &SignalRole::passive()},
      {parseagle::PinDirection::Input, &SignalRole::input()},
      {parseagle::PinDirection::Output, &SignalRole::output()},
      {parseagle::PinDirection::IO, &SignalRole::inout()},
      {parseagle::PinDirection::OpenCollector, &SignalRole::opendrain()},
      {parseagle::PinDirection::Power, &SignalRole::power()},
      {parseagle::PinDirection::Passive, &SignalRole::passive()},
      {parseagle::PinDirection::HighZ, &SignalRole::passive()},
      {parseagle::PinDirection::Supply, &SignalRole::passive()},
  };

  const QStringList key = {libName, libUrn, eagleDeviceSet.getName()};
  if (mComponentMap.contains(key)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Component> component(new Component(
      mSettings.createUuid(), mSettings.version, mSettings.author,
      mSettings.created,
      mTc->convertComponentName(mSettings.namePrefix +
                                eagleDeviceSet.getName()),
      mTc->convertElementDescription(eagleDeviceSet.getDescription()),
      mSettings.keywords));
  component->setGeneratedBy(generatedBy(libName, {eagleDeviceSet.getName()}));
  component->setCategories(mSettings.componentCategories);
  component->setPrefixes(NormDependentPrefixMap(
      mTc->convertComponentPrefix(eagleDeviceSet.getPrefix())));
  component->setDefaultValue(eagleDeviceSet.getUserValue()
                                 ? "{{ MPN }}"
                                 : "{{ MPN or DEVICE or COMPONENT }}");
  auto symbolVariant = std::make_shared<ComponentSymbolVariant>(
      mSettings.createUuid(), "", ElementName("default"), "");
  component->getSymbolVariants().append(symbolVariant);
  QHash<QString, int> pinCount;
  foreach (const auto& gate, eagleDeviceSet.getGates()) {
    const QStringList symbolKey = {libName, libUrn, gate.getSymbol()};
    for (auto pinIt = mSymbolPinMap[symbolKey].constBegin();
         pinIt != mSymbolPinMap[symbolKey].constEnd(); pinIt++) {
      pinCount[pinIt.key()]++;
    }
  }
  const bool addGateSuffixes = eagleDeviceSet.getGates().count() > 1;
  foreach (const auto& gate, eagleDeviceSet.getGates()) {
    const QStringList symbolKey = {libName, libUrn, gate.getSymbol()};
    const std::optional<Uuid> symbolUuid = mSymbolMap.value(symbolKey);
    if (!symbolUuid) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Dependent symbol \"%1\" not imported.").arg(gate.getSymbol()));
    }
    auto item = std::make_shared<ComponentSymbolVariantItem>(
        mSettings.createUuid(), *symbolUuid,
        mTc->convertPoint(gate.getPosition()), Angle(0), true,
        mTc->convertGateName(addGateSuffixes ? gate.getName() : ""));
    symbolVariant->getSymbolItems().append(item);
    for (auto pinIt = mSymbolPinMap[symbolKey].constBegin();
         pinIt != mSymbolPinMap[symbolKey].constEnd(); pinIt++) {
      Uuid signalUuid = mSettings.createUuid();
      QString signalName = pinIt.key();
      if ((pinCount[signalName] > 1) ||
          (component->getSignals().contains(signalName))) {
        // Name conflict -> add prefix to ensure unique signal names.
        signalName.prepend(*item->getSuffix() % "_");
      }
      const SignalRole& signalRole = *signalRoleMap.value(
          pinIt->first->getDirection(), &SignalRole::passive());
      const CmpSigPinDisplayType& displayType =
          *displayTypeMap.value(pinIt->first->getVisibility(),
                                &CmpSigPinDisplayType::componentSignal());
      const QString forcedNetName =
          (pinIt->first->getDirection() == parseagle::PinDirection::Supply)
          ? pinIt.key()
          : QString();
      const bool isRequired = false;
      const bool isNegated =
          (pinIt->first->getFunction() == parseagle::PinFunction::Dot) ||
          (pinIt->first->getFunction() == parseagle::PinFunction::DotClock);
      const bool isClock =
          (pinIt->first->getFunction() == parseagle::PinFunction::Clock) ||
          (pinIt->first->getFunction() == parseagle::PinFunction::DotClock);
      component->getSignals().append(std::make_shared<ComponentSignal>(
          signalUuid, mTc->convertPinOrPadName(signalName), signalRole,
          forcedNetName, isRequired, isNegated, isClock));
      item->getPinSignalMap().append(
          std::make_shared<ComponentPinSignalMapItem>(pinIt->second.value(),
                                                      signalUuid, displayType));
      mComponentSignalMap[key + QStringList{gate.getName(), pinIt.key()}] =
          signalUuid;
    }
  }
  // If the device set has no package at all, we consider it as a schematic-only
  // component to avoid the "unplaced devices" warning in the board editor.
  bool hasPackage = false;
  foreach (const auto& device, eagleDeviceSet.getDevices()) {
    if (!device.getPackage().isEmpty()) {
      hasPackage = true;
    }
  }
  component->setIsSchematicOnly(!hasPackage);
  mComponentMap[key] = component->getUuid();
  return component;
}

std::unique_ptr<Device> EagleLibraryConverter::createDevice(
    const QString& devLibName, const QString& devLibUrn,
    const parseagle::DeviceSet& eagleDeviceSet,
    const parseagle::Device& eagleDevice, const QString& pkgLibName,
    const QString& pkgLibUrn, MessageLogger& log) {
  Q_UNUSED(log);
  const QStringList componentKey = {devLibName, devLibUrn,
                                    eagleDeviceSet.getName()};
  const std::optional<Uuid> componentUuid = mComponentMap.value(componentKey);
  if (!componentUuid) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Dependent component \"%1\" not imported.")
                           .arg(eagleDeviceSet.getName()));
  }
  const QStringList packageKey = {pkgLibName, pkgLibUrn,
                                  eagleDevice.getPackage()};
  const std::optional<Uuid> packageUuid = mPackageMap.value(packageKey);
  if (!packageUuid) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Dependent package \"%1\" not imported.")
                           .arg(eagleDevice.getPackage()));
  }
  std::unique_ptr<Device> device(new Device(
      mSettings.createUuid(), mSettings.version, mSettings.author,
      mSettings.created,
      mTc->convertDeviceName(mSettings.namePrefix + eagleDeviceSet.getName(),
                             eagleDevice.getName()),
      mTc->convertElementDescription(eagleDeviceSet.getDescription()),
      mSettings.keywords, *componentUuid, *packageUuid));
  device->setGeneratedBy(generatedBy(
      devLibName, {eagleDeviceSet.getName(), eagleDevice.getName()}));
  device->setCategories(mSettings.deviceCategories);
  for (auto padIt = mPackagePadMap[packageKey].constBegin();
       padIt != mPackagePadMap[packageKey].constEnd(); padIt++) {
    std::optional<Uuid> signalUuid;
    foreach (const auto& connection, eagleDevice.getConnections()) {
      if (connection.getPads().contains(padIt.key())) {
        signalUuid = mComponentSignalMap[componentKey +
                                         QStringList{connection.getGate(),
                                                     connection.getPin()}];
      }
    }
    device->getPadSignalMap().append(
        std::make_shared<DevicePadSignalMapItem>(padIt->value(), signalUuid));
  }
  foreach (const auto& eagleTechnology, eagleDevice.getTechnologies()) {
    AttributeList attributes;
    mTc->tryConvertAttributes(eagleTechnology.getAttributes(), attributes, log);
    SimpleString mpn(""), manufacturer("");
    mTc->tryExtractMpnAndManufacturer(attributes, mpn, manufacturer);
    if (mpn->isEmpty()) {
      mpn = cleanSimpleString(eagleTechnology.getName());  // Good idea or not?
    }
    if (!mpn->isEmpty()) {
      auto part = std::make_shared<Part>(mpn, manufacturer, attributes);
      device->getParts().append(part);
    }
  }
  return device;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EagleLibraryConverter::tryOrLogError(std::function<void()> func,
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

}  // namespace eagleimport
}  // namespace librepcb
