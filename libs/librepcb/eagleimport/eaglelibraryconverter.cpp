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

/*******************************************************************************
 *  Struct EagleLibraryConverterSettings
 ******************************************************************************/

EagleLibraryConverterSettings::EagleLibraryConverterSettings() noexcept
  : namePrefix(),
    version(Version::fromString("0.1")),
    author("EAGLE Import"),
    keywords("eagle,import"),
    autoThtAnnularWidth(EagleTypeConverter::getDefaultAutoThtAnnularWidth()) {
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryConverter::EagleLibraryConverter(
    const EagleLibraryConverterSettings& settings, QObject* parent) noexcept
  : QObject(parent), mSettings(settings) {
}

EagleLibraryConverter::~EagleLibraryConverter() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Uuid EagleLibraryConverter::getComponentSignalOfSymbolPin(
    const QString& libName, const QString& devSetName, const QString& gateName,
    const QString& pinName) const {
  auto uuid = mComponentSignalMap[std::make_pair(libName, devSetName)][gateName]
                                 [pinName];
  if (!uuid) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Could not find component signal from pin name: %1")
            .arg(pinName));
  }
  return *uuid;
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
    const QString& libName, const parseagle::Symbol& eagleSymbol,
    MessageLogger& log) {
  const auto key = std::make_pair(libName, eagleSymbol.getName());
  if (mSymbolMap.contains(key)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Symbol> symbol(new Symbol(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertElementName(mSettings.namePrefix + eagleSymbol.getName()),
      C::convertElementDescription(eagleSymbol.getDescription()),
      mSettings.keywords));
  symbol->setCategories(mSettings.symbolCategories);

  QList<C::Geometry> geometries;
  tryOrLogError(
      [&]() {
        // Enable grab areas on closed polygons. However, don't do this for
        // sheet frames as it would look ugly. We guess that by the absence of
        // pins.
        const bool grabArea = !eagleSymbol.getPins().isEmpty();
        geometries +=
            C::convertAndJoinWires(eagleSymbol.getWires(), grabArea, log);
      },
      log);
  foreach (const auto& obj, eagleSymbol.getRectangles()) {
    geometries.append(C::convertRectangle(obj, true));
  }
  foreach (const auto& obj, eagleSymbol.getPolygons()) {
    geometries.append(C::convertPolygon(obj, true));
  }
  foreach (const auto& obj, eagleSymbol.getCircles()) {
    geometries.append(C::convertCircle(obj, true));
  }
  foreach (const auto& obj, eagleSymbol.getFrames()) {
    geometries.append(C::convertFrame(obj));
  }
  // Disable grab area on geometries located *within* another grab area to
  // avoid overlapping grab areas, but also to avoid triggering issue
  // https://github.com/LibrePCB/LibrePCB/issues/1278.
  std::sort(geometries.begin(), geometries.end(),
            [](const EagleTypeConverter::Geometry& a,
               const EagleTypeConverter::Geometry& b) {
              if (a.path.isClosed() != b.path.isClosed()) {
                return a.path.isClosed();
              }
              if (a.path.isClosed()) {
                return a.path.calcAreaOfStraightSegments() >
                    b.path.calcAreaOfStraightSegments();
              } else {
                return a.path.getTotalStraightLength() >
                    b.path.getTotalStraightLength();
              }
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
          if (auto o = C::tryConvertToSchematicCircle(g)) {
            symbol->getCircles().append(o);
          } else if (auto o = C::tryConvertToSchematicPolygon(g)) {
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
    if (auto lpObj = C::tryConvertSchematicText(obj)) {
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
          const auto pinObj = C::convertSymbolPin(obj);
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
    const QString& libName, const parseagle::Package& eaglePackage,
    MessageLogger& log) {
  const auto key = std::make_pair(libName, eaglePackage.getName());
  if (mPackageMap.contains(key)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Package> package(new Package(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertElementName(mSettings.namePrefix + eaglePackage.getName()),
      C::convertElementDescription(eaglePackage.getDescription()),
      mSettings.keywords, librepcb::Package::AssemblyType::Auto));
  package->setCategories(mSettings.packageCategories);
  auto footprint = std::make_shared<Footprint>(Uuid::createRandom(),
                                               ElementName("default"), "");
  package->getFootprints().append(footprint);

  QList<C::Geometry> geometries;
  tryOrLogError(
      [&]() {
        geometries +=
            C::convertAndJoinWires(eaglePackage.getWires(), false, log);
      },
      log);
  foreach (const auto& obj, eaglePackage.getRectangles()) {
    geometries.append(C::convertRectangle(obj, false));
  }
  foreach (const auto& obj, eaglePackage.getPolygons()) {
    geometries.append(C::convertPolygon(obj, false));
  }
  foreach (const auto& obj, eaglePackage.getCircles()) {
    geometries.append(C::convertCircle(obj, false));
  }
  foreach (const auto& g, geometries) {
    tryOrLogError(
        [&]() {
          const auto zones = C::tryConvertToBoardZones(g);
          if (!zones.isEmpty()) {
            foreach (const auto& o, zones) {
              footprint->getZones().append(o);
            }
          } else if (auto o = C::tryConvertToBoardCircle(g)) {
            footprint->getCircles().append(o);
          } else if (auto o = C::tryConvertToBoardPolygon(g)) {
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
    if (auto lpObj = C::tryConvertBoardText(obj)) {
      footprint->getStrokeTexts().append(lpObj);
    } else {
      log.warning(tr("Skipped text on layer %1 (%2).")
                      .arg(obj.getLayer())
                      .arg(C::getLayerName(obj.getLayer())));
    }
  }
  foreach (const auto& obj, eaglePackage.getHoles()) {
    tryOrLogError([&]() { footprint->getHoles().append(C::convertHole(obj)); },
                  log);
  }
  foreach (const auto& obj, eaglePackage.getThtPads()) {
    tryOrLogError(
        [&]() {
          auto pair = C::convertThtPad(obj, mSettings.autoThtAnnularWidth);
          package->getPads().append(pair.first);
          footprint->getPads().append(pair.second);
          mPackagePadMap[key][obj.getName()] = pair.first->getUuid();
        },
        log);
  }
  foreach (const auto& obj, eaglePackage.getSmtPads()) {
    tryOrLogError(
        [&]() {
          auto pair = C::convertSmtPad(obj);
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
        Uuid::createRandom(), Layer::topCourtyard(), UnsignedLength(0), false,
        false, courtyardZone->getOutline()));
    footprint->getZones().clear();
  }
  mPackageMap[key] = package->getUuid();
  return package;
}

std::unique_ptr<Component> EagleLibraryConverter::createComponent(
    const QString& libName, const parseagle::DeviceSet& eagleDeviceSet,
    MessageLogger& log) {
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

  const auto key = std::make_pair(libName, eagleDeviceSet.getName());
  if (mComponentMap.contains(key)) {
    throw LogicError(__FILE__, __LINE__, "Duplicate import.");
  }
  std::unique_ptr<Component> component(new Component(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertComponentName(mSettings.namePrefix + eagleDeviceSet.getName()),
      C::convertElementDescription(eagleDeviceSet.getDescription()),
      mSettings.keywords));
  component->setCategories(mSettings.componentCategories);
  component->setPrefixes(NormDependentPrefixMap(
      C::convertComponentPrefix(eagleDeviceSet.getPrefix())));
  component->setDefaultValue(eagleDeviceSet.getUserValue()
                                 ? "{{ MPN }}"
                                 : "{{ MPN or DEVICE or COMPONENT }}");
  auto symbolVariant = std::make_shared<ComponentSymbolVariant>(
      Uuid::createRandom(), "", ElementName("default"), "");
  component->getSymbolVariants().append(symbolVariant);
  QHash<QString, int> pinCount;
  foreach (const auto& gate, eagleDeviceSet.getGates()) {
    const auto symbolKey = std::make_pair(libName, gate.getSymbol());
    for (auto pinIt = mSymbolPinMap[symbolKey].constBegin();
         pinIt != mSymbolPinMap[symbolKey].constEnd(); pinIt++) {
      pinCount[pinIt.key()]++;
    }
  }
  const bool addGateSuffixes = eagleDeviceSet.getGates().count() > 1;
  foreach (const auto& gate, eagleDeviceSet.getGates()) {
    const auto symbolKey = std::make_pair(libName, gate.getSymbol());
    const tl::optional<Uuid> symbolUuid = mSymbolMap.value(symbolKey);
    if (!symbolUuid) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Dependent symbol \"%1\" not imported.").arg(gate.getSymbol()));
    }
    auto item = std::make_shared<ComponentSymbolVariantItem>(
        Uuid::createRandom(), *symbolUuid, C::convertPoint(gate.getPosition()),
        Angle(0), true,
        C::convertGateName(addGateSuffixes ? gate.getName() : ""));
    symbolVariant->getSymbolItems().append(item);
    for (auto pinIt = mSymbolPinMap[symbolKey].constBegin();
         pinIt != mSymbolPinMap[symbolKey].constEnd(); pinIt++) {
      Uuid signalUuid = Uuid::createRandom();
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
          signalUuid, C::convertPinOrPadName(signalName), signalRole,
          forcedNetName, isRequired, isNegated, isClock));
      item->getPinSignalMap().append(
          std::make_shared<ComponentPinSignalMapItem>(pinIt->second.value(),
                                                      signalUuid, displayType));
      mComponentSignalMap[key][gate.getName()][pinIt.key()] = signalUuid;
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
    const QString& libName, const parseagle::DeviceSet& eagleDeviceSet,
    const parseagle::Device& eagleDevice, MessageLogger& log) {
  Q_UNUSED(log);
  const auto componentKey = std::make_pair(libName, eagleDeviceSet.getName());
  const tl::optional<Uuid> componentUuid = mComponentMap.value(componentKey);
  if (!componentUuid) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Dependent component \"%1\" not imported.")
                           .arg(eagleDeviceSet.getName()));
  }
  const auto packageKey = std::make_pair(libName, eagleDevice.getPackage());
  const tl::optional<Uuid> packageUuid = mPackageMap.value(packageKey);
  if (!packageUuid) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Dependent package \"%1\" not imported.")
                           .arg(eagleDevice.getPackage()));
  }
  std::unique_ptr<Device> device(new Device(
      Uuid::createRandom(), mSettings.version, mSettings.author,
      C::convertDeviceName(mSettings.namePrefix + eagleDeviceSet.getName(),
                           eagleDevice.getName()),
      C::convertElementDescription(eagleDeviceSet.getDescription()),
      mSettings.keywords, *componentUuid, *packageUuid));
  device->setCategories(mSettings.deviceCategories);
  for (auto padIt = mPackagePadMap[packageKey].constBegin();
       padIt != mPackagePadMap[packageKey].constEnd(); padIt++) {
    tl::optional<Uuid> signalUuid;
    foreach (const auto& connection, eagleDevice.getConnections()) {
      if (connection.getPads().contains(padIt.key())) {
        signalUuid = mComponentSignalMap[componentKey][connection.getGate()]
                                        [connection.getPin()];
      }
    }
    device->getPadSignalMap().append(
        std::make_shared<DevicePadSignalMapItem>(padIt->value(), signalUuid));
  }
  foreach (const auto& eagleTechnology, eagleDevice.getTechnologies()) {
    AttributeList attributes;
    C::tryConvertAttributes(eagleTechnology.getAttributes(), attributes, log);
    SimpleString mpn(""), manufacturer("");
    C::tryExtractMpnAndManufacturer(attributes, mpn, manufacturer);
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
