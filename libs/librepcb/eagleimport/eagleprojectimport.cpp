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
#include "eagleprojectimport.h"

#include "eaglelibraryconverter.h"
#include "eagletypeconverter.h"

#include <librepcb/core/attribute/attrtypestring.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/boardnetsegmentsplitter.h>
#include <librepcb/core/project/board/drc/boarddesignrulechecksettings.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicnetsegmentsplitter.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/transform.h>
#include <parseagle/board/board.h>
#include <parseagle/schematic/schematic.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {

using C = EagleTypeConverter;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleProjectImport::EagleProjectImport(QObject* parent) noexcept
  : QObject(parent), mLogger(new MessageLogger(true)) {
}

EagleProjectImport::~EagleProjectImport() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int EagleProjectImport::getSheetCount() const noexcept {
  return mSchematic ? mSchematic->getSheets().count() : 0;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EagleProjectImport::reset() noexcept {
  mNetSignalMap.clear();
  mSchematicDirNames.clear();
  mComponentMap.clear();
  mLibDeviceMap.clear();
  mLibPackageMap.clear();
  mLibComponentGateMap.clear();
  mLibComponentMap.clear();
  mLibSymbolMap.clear();
  mDeviceSets.clear();
  mPackages.clear();
  mSymbols.clear();
  mBoard.reset();
  mSchematic.reset();
  mProjectName.clear();
  mLogger->clear();
}

QStringList EagleProjectImport::open(const FilePath& sch, const FilePath& brd) {
  reset();

  QStringList warnings;
  std::unique_ptr<parseagle::Schematic> schematic;
  std::unique_ptr<parseagle::Board> board;

  try {
    schematic.reset(new parseagle::Schematic(sch.toStr(), &warnings));
    if (schematic->getSheets().isEmpty()) {
      warnings.append(tr("Project contains no schematic sheets."));
    }
    if (!schematic->getModules().isEmpty()) {
      warnings.append(
          tr("Project contains modules which are not supported yet!"));
    }
    if (hasBuses(*schematic)) {
      warnings.append(
          tr("Project contains buses which are not supported yet!"));
    }
    if (brd.isValid()) {
      board.reset(new parseagle::Board(brd.toStr(), &warnings));
    }
  } catch (const std::exception& e) {
    qWarning() << "Failed to parse EAGLE project:" << e.what();
    throw RuntimeError(__FILE__, __LINE__, e.what());
  }

  mProjectName = sch.getCompleteBasename();
  mSchematic.reset(schematic.release());
  mBoard.reset(board.release());
  importLibraries(mSchematic->getLibraries(), false);
  if (mBoard) {
    importLibraries(mBoard->getLibraries(), true);
  }
  return warnings;
}

void EagleProjectImport::import(Project& project) {
  if (!mSchematic) {
    throw LogicError(__FILE__, __LINE__);
  }

  try {
    mLogger->info(tr("Importing project, this may take a moment..."));
    mLogger->info(tr("If you experience any issues with the import, please "
                     "<a href=\"%1\">let us know</a> so we can improve it.")
                      .arg("https://librepcb.org/help/"));

    // Try to apply the automatic THT annular width to get correct pad sizes in
    // footprints.
    EagleLibraryConverterSettings settings;
    try {
      if (auto r = tryGetDrcRatio("rvPadBottom", "rlMinPadBottom",
                                  "rlMaxPadBottom")) {
        settings.autoThtAnnularWidth = *r;
      }
    } catch (const Exception& e) {
      mLogger->critical(QString("Could not configure automatic pad sizes: %1")
                            .arg(e.getMsg()));
    }

    EagleLibraryConverter converter(settings, this);

    // Add components.
    foreach (const parseagle::Part& part, mSchematic->getParts()) {
      MessageLogger log(mLogger.get(), part.getName());
      const Component& libCmp = importLibraryComponent(
          converter, project.getLibrary(), part.getLibrary(),
          part.getLibraryUrn(), part.getDeviceSet());
      auto symbVar = libCmp.getSymbolVariants().value(0);
      if (!symbVar) throw LogicError(__FILE__, __LINE__);
      QString name = cleanCircuitIdentifier(part.getName());
      if (name.isEmpty()) {
        name = libCmp.getUuid().toStr().left(8);  // Not so nice...
      }
      ComponentInstance* cmp = new ComponentInstance(
          project.getCircuit(), Uuid::createRandom(), libCmp,
          symbVar->getUuid(), CircuitIdentifier(name));
      if (!part.getValue().isEmpty()) {
        cmp->setValue(part.getValue());
      }
      AttributeList attributes = cmp->getAttributes();
      C::tryConvertAttributes(part.getAttributes(), attributes, log);
      auto eagleDevSet = getDeviceSet(part.getLibrary(), part.getLibraryUrn(),
                                      part.getDeviceSet());
      const parseagle::Device eagleDev =
          getDevice(*eagleDevSet, part.getDevice());
      if (const parseagle::Technology* eagleTech =
              tryGetTechnology(eagleDev, part.getTechnology())) {
        C::tryConvertAttributes(eagleTech->getAttributes(), attributes, log);
        if ((!attributes.contains("MPN")) &&
            (!attributes.contains("MANUFACTURER_PART_NUMBER")) &&
            (!attributes.contains("PART_NUMBER")) &&
            (!eagleTech->getName().trimmed().isEmpty())) {
          // Memorize it since it could be an MPN.
          attributes.append(std::make_shared<Attribute>(
              AttributeKey("EAGLE_TECHNOLOGY"), AttrTypeString::instance(),
              eagleTech->getName(), nullptr));
        }
      }
      cmp->setAttributes(attributes);
      mComponentMap.insert(
          part.getName(),
          ComponentMap{part.getLibrary(), part.getLibraryUrn(),
                       part.getDeviceSet(), part.getDevice(), cmp->getUuid()});
      project.getCircuit().addComponentInstance(*cmp);
    }

    // Warn about unsupported objects.
    if (!mSchematic->getModules().isEmpty()) {
      mLogger->critical(
          tr("Skipped modules because they are not supported yet!"));
    }

    // Import schematics.
    foreach (const parseagle::Sheet& sheet, mSchematic->getSheets()) {
      importSchematic(project, converter, sheet);
    }

    // Import board, if given.
    if (mBoard) {
      importBoard(project, converter);
    }

    // Status messages.
    mLogger->info(
        tr("Imported %n schematic sheet(s). Please check the ERC messages in "
           "the schematic editor.",
           nullptr, mSchematic->getSheets().count()));
    if (mBoard) {
      mLogger->info(
          tr("Imported a board. Please run the DRC in the board editor and fix "
             "remaining issues manually."));
    }
  } catch (const Exception& e) {
    mLogger->critical(tr("Import failed:") % " " % e.getMsg());
    throw e;
  } catch (const std::exception& e) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Parser error: %1").arg(e.what()));
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

const Symbol& EagleProjectImport::importLibrarySymbol(
    EagleLibraryConverter& converter, ProjectLibrary& library,
    const QString& libName, const QString& libUrn, const QString& symName) {
  const QStringList key = {libName, libUrn, symName};
  auto it = mLibSymbolMap.find(key);
  if (it == mLibSymbolMap.end()) {
    auto eagleSymbol = getSymbol(libName, libUrn, symName);
    MessageLogger log(mLogger.get(), eagleSymbol->getName());
    std::unique_ptr<Symbol> sym =
        converter.createSymbol(libName, libUrn, *eagleSymbol, log);
    it = mLibSymbolMap.insert(key, sym->getUuid());
    library.addSymbol(*sym.release());
  }
  auto sym = library.getSymbol(*it);
  Q_ASSERT(sym);
  return *sym;
}

const Component& EagleProjectImport::importLibraryComponent(
    EagleLibraryConverter& converter, ProjectLibrary& library,
    const QString& libName, const QString& libUrn, const QString& devSetName) {
  const QStringList key = {libName, libUrn, devSetName};
  auto it = mLibComponentMap.find(key);
  if (it == mLibComponentMap.end()) {
    auto eagleDevSet = getDeviceSet(libName, libUrn, devSetName);
    foreach (const parseagle::Gate& eagleGate, eagleDevSet->getGates()) {
      importLibrarySymbol(converter, library, libName, libUrn,
                          eagleGate.getSymbol());
    }
    MessageLogger log(mLogger.get(), eagleDevSet->getName());
    std::unique_ptr<Component> cmp =
        converter.createComponent(libName, libUrn, *eagleDevSet, log);
    if ((cmp->getSymbolVariants().count() != 1) ||
        (cmp->getSymbolVariants().first()->getSymbolItems().count() !=
         eagleDevSet->getGates().count())) {
      throw LogicError(__FILE__, __LINE__);
    }
    for (int i = 0; i < eagleDevSet->getGates().count(); ++i) {
      const QString gateName = eagleDevSet->getGates().at(i).getName();
      const Uuid gateUuid =
          cmp->getSymbolVariants().first()->getSymbolItems().at(i)->getUuid();
      const QStringList key = {cmp->getUuid().toStr(), gateName};
      mLibComponentGateMap.insert(key, gateUuid);
    }
    it = mLibComponentMap.insert(key, cmp->getUuid());
    library.addComponent(*cmp.release());
  }
  auto cmp = library.getComponent(*it);
  Q_ASSERT(cmp);
  return *cmp;
}

const Package& EagleProjectImport::importLibraryPackage(
    EagleLibraryConverter& converter, ProjectLibrary& library,
    const QString& libName, const QString& libUrn, const QString& pkgName) {
  const QStringList key = {libName, libUrn, pkgName};
  auto it = mLibPackageMap.find(key);
  if (it == mLibPackageMap.end()) {
    auto eaglePackage = getPackage(libName, libUrn, pkgName);
    MessageLogger log(mLogger.get(), eaglePackage->getName());
    std::unique_ptr<Package> pkg =
        converter.createPackage(libName, libUrn, *eaglePackage, log);
    it = mLibPackageMap.insert(key, pkg->getUuid());
    library.addPackage(*pkg.release());
  }
  auto pkg = library.getPackage(*it);
  Q_ASSERT(pkg);
  return *pkg;
}

const Device& EagleProjectImport::importLibraryDevice(
    EagleLibraryConverter& converter, ProjectLibrary& library,
    const QString& libName, const QString& libUrn, const QString& devSetName,
    const QString& devName, const QString& pkgLibName,
    const QString& pkgLibUrn) {
  const QStringList key = {libName, libUrn, devSetName, devName};
  auto it = mLibDeviceMap.find(key);
  if (it == mLibDeviceMap.end()) {
    auto eagleDevSet = getDeviceSet(libName, libUrn, devSetName);
    const parseagle::Device& eagleDev = getDevice(*eagleDevSet, devName);
    importLibraryPackage(converter, library, pkgLibName, pkgLibUrn,
                         eagleDev.getPackage());
    MessageLogger log(mLogger.get(), eagleDev.getName());
    std::unique_ptr<Device> dev = converter.createDevice(
        libName, libUrn, *eagleDevSet, eagleDev, pkgLibName, pkgLibUrn, log);
    it = mLibDeviceMap.insert(key, dev->getUuid());
    library.addDevice(*dev.release());
  }
  auto dev = library.getDevice(*it);
  Q_ASSERT(dev);
  return *dev;
}

void EagleProjectImport::importLibraries(const QList<parseagle::Library>& libs,
                                         bool isBoard) {
  foreach (const parseagle::Library& lib, libs) {
    const QStringList libKey{lib.getEmbeddedName(), lib.getEmbeddedUrn()};
    foreach (const parseagle::Symbol& sym, lib.getSymbols()) {
      const auto key = libKey + QStringList{sym.getName()};
      if ((!mSymbols.contains(key)) || (!isBoard)) {
        mSymbols[key] = std::make_shared<parseagle::Symbol>(sym);
      }
    }
    foreach (const parseagle::Package& pkg, lib.getPackages()) {
      const auto key = libKey + QStringList{pkg.getName()};
      if ((!mPackages.contains(key)) || (isBoard)) {
        mPackages[key] = std::make_shared<parseagle::Package>(pkg);
      }
    }
    foreach (const parseagle::DeviceSet& dev, lib.getDeviceSets()) {
      const auto key = libKey + QStringList{dev.getName()};
      if ((!mDeviceSets.contains(key)) || (!isBoard)) {
        mDeviceSets[key] = std::make_shared<parseagle::DeviceSet>(dev);
      }
    }
  }
}

void EagleProjectImport::importSchematic(Project& project,
                                         EagleLibraryConverter& converter,
                                         const parseagle::Sheet& sheet) {
  // Determine directory name.
  QString dirName = FilePath::cleanFileName(
      sheet.getDescription(), FilePath::ReplaceSpaces | FilePath::ToLowerCase);
  if (dirName.startsWith("sheet_")) {
    dirName.clear();  // Avoid conflicts!
  }
  if (dirName.isEmpty()) {
    dirName = QString("sheet_%1").arg(mSchematicDirNames.count() + 1);
  }
  mSchematicDirNames.insert(dirName);

  // Determine schematic name.
  QString name = cleanElementName(sheet.getDescription());
  if (name.isEmpty()) {
    // No translation to avoid exceptions!
    name = QString("Sheet %1").arg(mSchematicDirNames.count());
  }

  // Create schematic.
  MessageLogger log(mLogger.get(), name);
  Schematic* schematic = new librepcb::Schematic(
      project,
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory()),
      dirName, Uuid::createRandom(),
      ElementName(name));  // can throw
  project.addSchematic(*schematic);

  // Grid settings
  PositiveLength gridInterval = schematic->getGridInterval();
  LengthUnit gridUnit = schematic->getGridUnit();
  C::convertGrid(mSchematic->getGrid(), gridInterval, gridUnit);
  schematic->setGridInterval(gridInterval);
  schematic->setGridUnit(gridUnit);

  // Symbols
  foreach (const parseagle::Instance& eagleInst, sheet.getInstances()) {
    auto cmpIt = mComponentMap.find(eagleInst.getPart());
    if (cmpIt == mComponentMap.end()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Component instance not found: %1").arg(eagleInst.getPart()));
    }
    ComponentInstance* cmpInst =
        project.getCircuit().getComponentInstanceByUuid(cmpIt->uuid);
    if (!cmpInst) throw LogicError(__FILE__, __LINE__);
    auto gateIt = mLibComponentGateMap.find(QStringList{
        cmpInst->getLibComponent().getUuid().toStr(), eagleInst.getGate()});
    if (gateIt == mLibComponentGateMap.end()) {
      throw LogicError(__FILE__, __LINE__);
    }
    const bool mirror = eagleInst.getRotation().getMirror();
    const Angle rotation =
        C::convertAngle(mirror ? -eagleInst.getRotation().getAngle()
                               : eagleInst.getRotation().getAngle());
    SI_Symbol* symInst =
        new SI_Symbol(*schematic, Uuid::createRandom(), *cmpInst, *gateIt,
                      C::convertPoint(eagleInst.getPosition()), rotation,
                      mirror, !eagleInst.getSmashed());
    if (eagleInst.getSmashed()) {
      foreach (const parseagle::Attribute& eagleAttr,
               eagleInst.getAttributes()) {
        if (eagleAttr.getDisplay() != parseagle::AttributeDisplay::Value) {
          continue;
        }
        try {
          if (auto lpObj = C::tryConvertSchematicAttribute(eagleAttr)) {
            SI_Text* obj = new SI_Text(*schematic, *lpObj);
            symInst->addText(*obj);
          } else {
            log.warning(tr("Skipped text on layer %1 (%2).")
                            .arg(eagleAttr.getLayer())
                            .arg(C::getLayerName(eagleAttr.getLayer())));
          }
        } catch (const Exception& e) {
          log.warning(QString("Skipped attribute text: %1").arg(e.getMsg()));
        }
      }
      const Transform transform(*symInst);
      for (const Text& text : symInst->getLibSymbol().getTexts()) {
        if (!text.getText().startsWith("{{")) {
          Text copy(text);
          copy.setPosition(transform.map(copy.getPosition()));
          copy.setRotation(transform.mapNonMirrorable(copy.getRotation()));
          if (symInst->getMirrored()) {
            copy.setAlign(copy.getAlign().mirroredV());
          }
          SI_Text* obj = new SI_Text(*schematic, copy);
          symInst->addText(*obj);
        }
      }
    }
    schematic->addSymbol(*symInst);
  }

  // Geometry
  QList<C::Geometry> geometries;
  try {
    geometries += C::convertAndJoinWires(sheet.getWires(), true, log);
  } catch (const Exception& e) {
    log.warning(QString("Failed to join wires: %1").arg(e.getMsg()));
  }
  foreach (const parseagle::Rectangle& eagleObj, sheet.getRectangles()) {
    geometries.append(C::convertRectangle(eagleObj, true));
  }
  foreach (const parseagle::Polygon& eagleObj, sheet.getPolygons()) {
    geometries.append(C::convertPolygon(eagleObj, true));
  }
  foreach (const parseagle::Circle& eagleObj, sheet.getCircles()) {
    geometries.append(C::convertCircle(eagleObj, true));
  }
  foreach (const parseagle::Frame& eagleObj, sheet.getFrames()) {
    geometries.append(C::convertFrame(eagleObj));
  }
  foreach (const auto& g, geometries) {
    if (auto o = C::tryConvertToSchematicPolygon(g)) {
      o->setIsGrabArea(g.grabArea && o->isFilled());
      SI_Polygon* obj = new SI_Polygon(*schematic, *o);
      schematic->addPolygon(*obj);
    } else {
      log.warning(tr("Skipped graphics object on layer %1 (%2).")
                      .arg(g.layerId)
                      .arg(C::getLayerName(g.layerId)));
    }
  }

  // Texts
  foreach (const parseagle::Text& eagleObj, sheet.getTexts()) {
    try {
      if (auto lpObj = C::tryConvertSchematicText(eagleObj)) {
        SI_Text* obj = new SI_Text(*schematic, *lpObj);
        schematic->addText(*obj);
      } else {
        log.warning(tr("Skipped text on layer %1 (%2).")
                        .arg(eagleObj.getLayer())
                        .arg(C::getLayerName(eagleObj.getLayer())));
      }
    } catch (const Exception& e) {
      log.warning(QString("Skipped text: %1").arg(e.getMsg()));
    }
  }

  // Nets
  foreach (const parseagle::Net& eagleNet, sheet.getNets()) {
    NetSignal& netSignal = importNet(project, eagleNet);
    try {
      SchematicNetSegmentSplitter splitter;
      QMap<std::pair<Uuid, Uuid>, SI_SymbolPin*> pinMap;
      QHash<Uuid, SI_NetPoint*> netPointMap;
      QHash<Point, NetLineAnchor> anchorMap;

      // Collect pin refs.
      foreach (const parseagle::Segment& eagleSegment, eagleNet.getSegments()) {
        foreach (const parseagle::PinRef& eaglePinRef,
                 eagleSegment.getPinRefs()) {
          auto cmpIt = mComponentMap.find(eaglePinRef.getPart());
          if (cmpIt == mComponentMap.end()) {
            throw LogicError(__FILE__, __LINE__, "Component not found.");
          }
          ComponentInstance* cmpInst =
              project.getCircuit().getComponentInstanceByUuid(cmpIt->uuid);
          if (!cmpInst) {
            throw LogicError(__FILE__, __LINE__, "Component not found.");
          }
          const parseagle::Part& part = getPart(eaglePinRef.getPart());
          const Uuid sigUuid = converter.getComponentSignalOfSymbolPin(
              part.getLibrary(), part.getLibraryUrn(), part.getDeviceSet(),
              eaglePinRef.getGate(), eaglePinRef.getPin());
          ComponentSignalInstance* cmpSigInst =
              cmpInst->getSignalInstance(sigUuid);
          if (!cmpSigInst) {
            throw LogicError(__FILE__, __LINE__, "Component signal not found.");
          }
          cmpSigInst->setNetSignal(&netSignal);
          if (cmpSigInst->isNetSignalNameForced()) {
            netSignal.setName(netSignal.getName(), false);  // No auto-name
          }
          if (cmpSigInst->getRegisteredSymbolPins().count() != 1) {
            throw LogicError(__FILE__, __LINE__,
                             "unexpected symbol pin count.");
          }
          auto symbolPin = cmpSigInst->getRegisteredSymbolPins().first();
          pinMap.insert(std::make_pair(symbolPin->getSymbol().getUuid(),
                                       symbolPin->getLibPinUuid()),
                        symbolPin);
          const NetLineAnchor pinAnchor = NetLineAnchor::pin(
              symbolPin->getSymbol().getUuid(), symbolPin->getLibPinUuid());
          splitter.addSymbolPin(pinAnchor, symbolPin->getPosition());
          auto it = anchorMap.find(symbolPin->getPosition());
          if (it != anchorMap.end()) {
            // There's another pin on the same position -> add a netline to
            // connect them since EAGLE implicitly consider them as connected.
            splitter.addNetLine(NetLine(
                Uuid::createRandom(), UnsignedLength(158750), *it, pinAnchor));
          }
          anchorMap.insert(symbolPin->getPosition(), pinAnchor);
        }

        // Convert wires.
        auto getOrCreateAnchor = [&anchorMap, &splitter](const Point& pos) {
          auto it = anchorMap.find(pos);
          if (it == anchorMap.end()) {
            const Junction junction(Uuid::createRandom(), pos);
            splitter.addJunction(junction);
            it = anchorMap.insert(pos,
                                  NetLineAnchor::junction(junction.getUuid()));
          }
          return *it;
        };
        foreach (const parseagle::Wire& eagleWire, eagleSegment.getWires()) {
          // Skip zero-length wires to avoid possible redundant wires.
          if (eagleWire.getP1() == eagleWire.getP2()) {
            continue;
          }
          splitter.addNetLine(
              NetLine(Uuid::createRandom(), UnsignedLength(158750),
                      getOrCreateAnchor(C::convertPoint(eagleWire.getP1())),
                      getOrCreateAnchor(C::convertPoint(eagleWire.getP2()))));
          if (eagleWire.getWireStyle() != parseagle::WireStyle::Continuous) {
            log.warning(
                tr("Dashed/dotted line is not supported, converting to "
                   "continuous."));
          }
          if (eagleWire.getWireCap() != parseagle::WireCap::Round) {
            log.warning(
                tr("Flat line end is not supported, converting to round."));
          }
        }
        foreach (const parseagle::Label& eagleLabel, eagleSegment.getLabels()) {
          const Angle rot =
              C::convertAngle(eagleLabel.getRotation().getAngle());
          const bool mirror = eagleLabel.getRotation().getMirror();
          Point pos = C::convertPoint(eagleLabel.getPosition());
          if (eagleLabel.getXref()) {
            pos += Point(mirror ? -254000 : 254000, -1270000)
                       .rotated(mirror ? -rot : rot);
            log.warning(
                tr("XRef-style net label is not supported, converting to "
                   "normal net label."));
          }
          splitter.addNetLabel(
              NetLabel(Uuid::createRandom(), pos, mirror ? -rot : rot, mirror));
        }
      }

      // Determine segments and add them to the schematic.
      auto getAnchor = [&pinMap, &netPointMap](const NetLineAnchor& anchor) {
        if (auto pin = anchor.tryGetPin()) {
          auto pinPtr = pinMap.value(std::make_pair(pin->symbol, pin->pin));
          if (pinPtr) {
            return static_cast<SI_NetLineAnchor*>(pinPtr);
          }
        } else if (auto junction = anchor.tryGetJunction()) {
          if (auto netPoint = netPointMap.value(*junction)) {
            return static_cast<SI_NetLineAnchor*>(netPoint);
          }
        }
        throw LogicError(__FILE__, __LINE__, "Unknown net line anchor.");
      };
      foreach (const auto& segment, splitter.split()) {
        SI_NetSegment* netSegment =
            new SI_NetSegment(*schematic, Uuid::createRandom(), netSignal);
        schematic->addNetSegment(*netSegment);
        QList<SI_NetPoint*> netPoints;
        QList<SI_NetLine*> netLines;
        for (const Junction& junction : segment.junctions) {
          SI_NetPoint* np = new SI_NetPoint(*netSegment, junction.getUuid(),
                                            junction.getPosition());
          netPointMap.insert(np->getUuid(), np);
          netPoints.append(np);
        }
        for (const NetLine& line : segment.netlines) {
          netLines.append(new SI_NetLine(
              *netSegment, line.getUuid(), *getAnchor(line.getStartPoint()),
              *getAnchor(line.getEndPoint()), line.getWidth()));
        }
        netSegment->addNetPointsAndNetLines(netPoints, netLines);
        for (const NetLabel& label : segment.netlabels) {
          SI_NetLabel* nl = new SI_NetLabel(*netSegment, label);
          netSegment->addNetLabel(*nl);
          netSignal.setName(netSignal.getName(), false);  // No auto-name
        }
      }
    } catch (const Exception& e) {
      log.critical(QString("Failed to import segment of net '%1': %2")
                       .arg(eagleNet.getName(), e.getMsg()));
    }
  }

  // Warn about unsupported objects.
  if (!sheet.getBuses().isEmpty()) {
    log.critical(tr("Skipped %n bus(es) because they are not supported yet!",
                    nullptr, sheet.getBuses().count()));
  }
}

NetSignal& EagleProjectImport::importNet(Project& project,
                                         const parseagle::Net& net) {
  auto it = mNetSignalMap.find(net.getName());
  if (it == mNetSignalMap.end()) {
    const Uuid uuid = Uuid::createRandom();
    QString name =
        C::convertInversionSyntax(cleanCircuitIdentifier(net.getName()));
    if (name.isEmpty()) {
      name = uuid.toStr().left(8);
    } else if (project.getCircuit().getNetSignalByName(name)) {
      name = name.left(20) % "_" % uuid.toStr().left(8);
    }
    if (project.getCircuit().getNetClasses().count() != 1) {
      throw LogicError(__FILE__, __LINE__, "Unexpected count of net classes.");
    }
    NetSignal* netSignal =
        new NetSignal(project.getCircuit(), uuid,
                      *project.getCircuit().getNetClasses().first(),
                      CircuitIdentifier(name), true);
    project.getCircuit().addNetSignal(*netSignal);
    it = mNetSignalMap.insert(net.getName(), uuid);
  }
  NetSignal* netSignal = project.getCircuit().getNetSignals().value(*it);
  Q_ASSERT(netSignal);
  return *netSignal;
}

void EagleProjectImport::importBoard(Project& project,
                                     EagleLibraryConverter& converter) {
  // Create board.
  MessageLogger log(mLogger.get(), "BOARD");
  Board* board = new librepcb::Board(
      project,
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory()),
      "default", Uuid::createRandom(),
      ElementName("default"));  // can throw
  project.addBoard(*board);

  // Grid settings
  PositiveLength gridInterval = board->getGridInterval();
  LengthUnit gridUnit = board->getGridUnit();
  C::convertGrid(mBoard->getGrid(), gridInterval, gridUnit);
  board->setGridInterval(gridInterval);
  board->setGridUnit(gridUnit);

  // Layer setup
  QHash<const Layer*, const Layer*> copperLayerMap;
  if (auto p = mBoard->getDesignRules().tryGetParam("layerSetup")) {
    copperLayerMap = C::convertLayerSetup(p->getValue());
  }
  copperLayerMap.insert(&Layer::topCopper(), &Layer::topCopper());
  copperLayerMap.insert(&Layer::botCopper(), &Layer::botCopper());
  board->setInnerLayerCount(copperLayerMap.count() - 2);
  // Warning: With automatic return type deduction, the application crashes?!?!
  auto mapLayer = [&copperLayerMap](const Layer* layer) -> const Layer* {
    return copperLayerMap.value(layer, layer);
  };

  // Design rules
  UnsignedLength minViaAnnularWidth(0);
  try {
    BoardDesignRules r = board->getDesignRules();
    r.setPadCmpSideAutoAnnularRing(false);  // Not sure if EAGLE supports this.
    r.setPadInnerAutoAnnularRing(true);  // Not sure if EAGLE supports this.
    if (auto p = mBoard->getDesignRules().tryGetParam("mlViaStopLimit")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      r.setStopMaskMaxViaDiameter(value);
    }
    if (board->getInnerLayerCount() > 0) {
      // Take automatic THT annular width from inner layers since it is probably
      // the smaller one and thus should reduce the risk of DRC warnings.
      if (auto v =
              tryGetDrcRatio("rvPadInner", "rlMinPadInner", "rlMaxPadInner")) {
        r.setPadAnnularRing(*v);
      }
    } else {
      // Take automatic THT annular width from bottom layer since inner layers
      // are not disabled so these values might not be reasonable.
      if (auto v = tryGetDrcRatio("rvPadBottom", "rlMinPadBottom",
                                  "rlMaxPadBottom")) {
        r.setPadAnnularRing(*v);
      }
    }
    if (auto v =
            tryGetDrcRatio("rvViaOuter", "rlMinViaOuter", "rlMaxViaOuter")) {
      r.setViaAnnularRing(*v);
      minViaAnnularWidth = v->getMinValue();
    }
    if (auto v =
            tryGetDrcRatio("mvStopFrame", "mlMinStopFrame", "mlMaxStopFrame")) {
      r.setStopMaskClearance(*v);
    }
    if (auto v = tryGetDrcRatio("mvCreamFrame", "mlMinCreamFrame",
                                "mlMaxCreamFrame")) {
      r.setSolderPasteClearance(*v);
    }
    board->setDesignRules(r);
  } catch (const Exception& e) {
    log.critical(QString("Failed to import design rules: %1").arg(e.getMsg()));
  }

  // DRC settings
  try {
    BoardDesignRuleCheckSettings s = board->getDrcSettings();
    if (auto p = mBoard->getDesignRules().tryGetParam("mdWireWire")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      s.setMinCopperCopperClearance(value);
    }
    if (auto p = mBoard->getDesignRules().tryGetParam("mdCopperDimension")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      s.setMinCopperBoardClearance(value);
      s.setMinCopperNpthClearance(value);
    }
    if (auto p = mBoard->getDesignRules().tryGetParam("mdDrill")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      s.setMinDrillDrillClearance(value);
      s.setMinDrillBoardClearance(value);
    }
    if (auto p = mBoard->getDesignRules().tryGetParam("msWidth")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      s.setMinCopperWidth(value);
    }
    if (auto p = mBoard->getDesignRules().tryGetParam("rlMinViaInner")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      s.setMinPthAnnularRing(value);
    }
    if (auto p = mBoard->getDesignRules().tryGetParam("msDrill")) {
      const auto value = C::convertParamTo<UnsignedLength>(*p);
      s.setMinNpthDrillDiameter(value);
      s.setMinPthDrillDiameter(value);
    }
    board->setDrcSettings(s);
  } catch (const Exception& e) {
    log.critical(QString("Failed to import DRC settings: %1").arg(e.getMsg()));
  }

  // Devices
  foreach (const parseagle::Element& eagleElem, mBoard->getElements()) {
    auto cmpIt = mComponentMap.find(eagleElem.getName());
    if (cmpIt == mComponentMap.end()) {
      log.critical(QString("Component '%1' (%2) not found in circuit. Note "
                           "that LibrePCB does not yet support placing devices "
                           "on the board which don't exist in the schematic.")
                       .arg(eagleElem.getName())
                       .arg(eagleElem.getPackage()));
      continue;
    }
    ComponentInstance* cmpInst =
        project.getCircuit().getComponentInstanceByUuid(cmpIt->uuid);
    if (!cmpInst) throw LogicError(__FILE__, __LINE__);
    const Package& libPkg = importLibraryPackage(
        converter, project.getLibrary(), eagleElem.getLibrary(),
        eagleElem.getLibraryUrn(), eagleElem.getPackage());
    if (libPkg.getFootprints().count() != 1) {
      throw LogicError(__FILE__, __LINE__);
    }
    const Device& libDev =
        importLibraryDevice(converter, project.getLibrary(), cmpIt->libName,
                            cmpIt->libUrn, cmpIt->devSetName, cmpIt->devName,
                            eagleElem.getLibrary(), eagleElem.getLibraryUrn());
    const bool mirror = eagleElem.getRotation().getMirror();
    const Angle rotation = C::convertAngle(eagleElem.getRotation().getAngle());
    BI_Device* devInst = new BI_Device(
        *board, *cmpInst, libDev.getUuid(),
        libPkg.getFootprints().first()->getUuid(),
        C::convertPoint(eagleElem.getPosition()), mirror ? -rotation : rotation,
        mirror, eagleElem.getLocked(), true, !eagleElem.getSmashed());
    board->addDeviceInstance(*devInst);

    // Add stroke texts.
    if (eagleElem.getSmashed()) {
      foreach (const parseagle::Attribute& eagleAttr,
               eagleElem.getAttributes()) {
        if (eagleAttr.getDisplay() != parseagle::AttributeDisplay::Value) {
          continue;
        }
        try {
          if (auto lpObj = C::tryConvertBoardAttribute(eagleAttr)) {
            BI_StrokeText* obj = new BI_StrokeText(
                *board,
                BoardStrokeTextData(
                    lpObj->getUuid(), lpObj->getLayer(), lpObj->getText(),
                    lpObj->getPosition(), lpObj->getRotation(),
                    lpObj->getHeight(), lpObj->getStrokeWidth(),
                    lpObj->getLetterSpacing(), lpObj->getLineSpacing(),
                    lpObj->getAlign(), lpObj->getMirrored(),
                    lpObj->getAutoRotate(), false));
            devInst->addStrokeText(*obj);
          } else {
            log.warning(tr("Skipped text on layer %1 (%2).")
                            .arg(eagleAttr.getLayer())
                            .arg(C::getLayerName(eagleAttr.getLayer())));
          }
        } catch (const Exception& e) {
          log.warning(QString("Skipped attribute text: %1").arg(e.getMsg()));
        }
      }
      const Transform transform(*devInst);
      for (const StrokeText& text :
           devInst->getLibFootprint().getStrokeTexts()) {
        if (!text.getText().startsWith("{{")) {
          BI_StrokeText* obj = new BI_StrokeText(
              *board,
              BoardStrokeTextData(
                  text.getUuid(), text.getLayer(), text.getText(),
                  transform.map(text.getPosition()),
                  transform.mapMirrorable(text.getRotation()), text.getHeight(),
                  text.getStrokeWidth(), text.getLetterSpacing(),
                  text.getLineSpacing(), text.getAlign(),
                  devInst->getMirrored() != text.getMirrored(),
                  text.getAutoRotate(), false));
          devInst->addStrokeText(*obj);
        }
      }
    }

    // Add assembly options.
    AttributeList attributes = cmpInst->getAttributes();
    auto assemblyOption = std::make_shared<ComponentAssemblyOption>(
        libDev.getUuid(), AttributeList(), QSet<Uuid>(), PartList());
    if (eagleElem.getPopulate()) {
      assemblyOption->setAssemblyVariants(
          project.getCircuit().getAssemblyVariants().getUuidSet());
    }
    SimpleString mpn(""), manufacturer("");
    C::tryExtractMpnAndManufacturer(attributes, mpn, manufacturer);
    if (!mpn->isEmpty()) {
      // Convert attributes to a Part.
      assemblyOption->getParts().append(
          std::make_shared<Part>(mpn, manufacturer, AttributeList()));
      cmpInst->setAttributes(attributes);
    }
    cmpInst->setAssemblyOptions(ComponentAssemblyOptionList{assemblyOption});
  }

  // Geometry
  QList<C::Geometry> geometries;
  try {
    geometries += C::convertAndJoinWires(mBoard->getWires(), true, log);
  } catch (const Exception& e) {
    log.warning(QString("Failed to join wires: %1").arg(e.getMsg()));
  }
  foreach (const parseagle::Rectangle& eagleObj, mBoard->getRectangles()) {
    geometries.append(C::convertRectangle(eagleObj, true));
  }
  foreach (const parseagle::Polygon& eagleObj, mBoard->getPolygons()) {
    geometries.append(C::convertPolygon(eagleObj, true));
  }
  foreach (const parseagle::Circle& eagleObj, mBoard->getCircles()) {
    geometries.append(C::convertCircle(eagleObj, true));
  }
  foreach (const auto& g, geometries) {
    const auto zones = C::tryConvertToBoardZones(g);
    if (!zones.isEmpty()) {
      foreach (const auto zone, zones) {
        QSet<const Layer*> layers;
        if (zone->getLayers().testFlag(Zone::Layer::Top)) {
          layers.insert(&Layer::topCopper());
        }
        if (zone->getLayers().testFlag(Zone::Layer::Inner)) {
          for (int i = 1; i <= board->getInnerLayerCount(); ++i) {
            layers.insert(Layer::innerCopper(i));
          }
        }
        if (zone->getLayers().testFlag(Zone::Layer::Bottom)) {
          layers.insert(&Layer::botCopper());
        }
        BI_Zone* obj =
            new BI_Zone(*board,
                        BoardZoneData(zone->getUuid(), layers, zone->getRules(),
                                      zone->getOutline(), false));
        board->addZone(*obj);
      }
    } else if (auto o = C::tryConvertToBoardPolygon(g)) {
      o->setIsGrabArea(g.grabArea && o->isFilled());
      BI_Polygon* obj = new BI_Polygon(
          *board,
          BoardPolygonData(o->getUuid(), *mapLayer(&o->getLayer()),
                           o->getLineWidth(), o->getPath(), o->isFilled(),
                           o->isGrabArea(), false));
      board->addPolygon(*obj);
    } else {
      log.warning(tr("Skipped graphics object on layer %1 (%2).")
                      .arg(g.layerId)
                      .arg(C::getLayerName(g.layerId)));
    }
  }

  // Texts
  foreach (const parseagle::Text& eagleObj, mBoard->getTexts()) {
    try {
      if (auto lpObj = C::tryConvertBoardText(eagleObj)) {
        BI_StrokeText* obj = new BI_StrokeText(
            *board,
            BoardStrokeTextData(
                lpObj->getUuid(), *mapLayer(&lpObj->getLayer()),
                lpObj->getText(), lpObj->getPosition(), lpObj->getRotation(),
                lpObj->getHeight(), lpObj->getStrokeWidth(),
                lpObj->getLetterSpacing(), lpObj->getLineSpacing(),
                lpObj->getAlign(), lpObj->getMirrored(), lpObj->getAutoRotate(),
                false));
        board->addStrokeText(*obj);
      } else {
        log.warning(tr("Skipped text on layer %1 (%2).")
                        .arg(eagleObj.getLayer())
                        .arg(C::getLayerName(eagleObj.getLayer())));
      }
    } catch (const Exception& e) {
      log.warning(QString("Skipped text: %1").arg(e.getMsg()));
    }
  }

  // Holes
  foreach (const parseagle::Hole& eagleObj, mBoard->getHoles()) {
    try {
      std::shared_ptr<Hole> lpObj = C::convertHole(eagleObj);
      BI_Hole* obj = new BI_Hole(
          *board,
          BoardHoleData(lpObj->getUuid(), lpObj->getDiameter(),
                        lpObj->getPath(), lpObj->getStopMaskConfig(), false));
      board->addHole(*obj);
    } catch (const Exception& e) {
      log.critical(QString("Skipped hole: %1").arg(e.getMsg()));
    }
  }

  // Make the longest polygon the board outline, and all others cutouts.
  // This logic is needed because EAGLE does not distinguish these two layers.
  BI_Polygon* outlineCandidate = nullptr;
  foreach (auto polygon, board->getPolygons()) {
    if ((polygon->getData().getLayer() == Layer::boardOutlines()) ||
        (polygon->getData().getLayer() == Layer::boardCutouts())) {
      if ((!outlineCandidate) ||
          (polygon->getData().getPath().getTotalStraightLength() >
           outlineCandidate->getData().getPath().getTotalStraightLength())) {
        outlineCandidate = polygon;
      }
      polygon->setLayer(Layer::boardCutouts());
    }
  }
  if (outlineCandidate) {
    outlineCandidate->setLayer(Layer::boardOutlines());
  }

  // Net segments
  foreach (const parseagle::Signal& eagleSignal, mBoard->getSignals()) {
    // Find net signal.
    NetSignal* netSignal = nullptr;
    auto it = mNetSignalMap.find(eagleSignal.getName());
    if (it != mNetSignalMap.end()) {
      netSignal = project.getCircuit().getNetSignals().value(*it);
      if (!netSignal) {
        throw LogicError(__FILE__, __LINE__);
      }
    }

    try {
      BoardNetSegmentSplitter splitter;
      QMap<std::pair<Uuid, Uuid>, BI_FootprintPad*> padMap;
      QHash<Uuid, BI_Via*> viaMap;
      QHash<Uuid, BI_NetPoint*> netPointMap;
      QHash<std::pair<const Layer*, Point>, TraceAnchor> anchorMap;

      // Convert vias.
      foreach (const parseagle::Via& eagleVia, eagleSignal.getVias()) {
        int startLayerId = 0, endLayerId = 0;
        const bool validLayers = eagleVia.tryGetStartLayer(startLayerId) &&
            eagleVia.tryGetEndLayer(endLayerId);
        const Layer* startLayer =
            mapLayer(C::tryConvertBoardLayer(startLayerId));
        const Layer* endLayer = mapLayer(C::tryConvertBoardLayer(endLayerId));
        if ((!validLayers) || (!startLayer) || (!endLayer) ||
            (!startLayer->isCopper()) || (!endLayer->isCopper()) ||
            (startLayer->getCopperNumber() > endLayer->getCopperNumber())) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Invalid via layer extent: %1")
                                 .arg(eagleVia.getExtent()));
        }
        const PositiveLength drillDiameter(
            C::convertLength(eagleVia.getDrill()));
        const Length size = std::max(C::convertLength(eagleVia.getDiameter()),
                                     drillDiameter + minViaAnnularWidth * 2);
        const MaskConfig stopMaskConfig = eagleVia.getAlwaysStop()
            ? MaskConfig::automatic()
            : MaskConfig::off();
        if (eagleVia.getShape() != parseagle::ViaShape::Round) {
          log.warning(
              tr("Square/octagon via shape not supported, converting to "
                 "circular."));
        }
        const Via via(Uuid::createRandom(), *startLayer, *endLayer,
                      C::convertPoint(eagleVia.getPosition()),
                      PositiveLength(size), drillDiameter, stopMaskConfig);
        splitter.addVia(via, false);
        const TraceAnchor viaAnchor = TraceAnchor::via(via.getUuid());
        anchorMap.insert(std::make_pair(nullptr, via.getPosition()), viaAnchor);
      }

      // Convert traces.
      auto getOrCreateAnchor = [&padMap, &anchorMap, &splitter, netSignal](
                                   const Layer& layer, const Point& pos) {
        // Find via.
        auto it = anchorMap.find(std::make_pair(nullptr, pos));
        // Find net point.
        if (it == anchorMap.end()) {
          it = anchorMap.find(std::make_pair(&layer, pos));
        }
        // Find pad.
        if ((it == anchorMap.end()) && netSignal) {
          foreach (ComponentSignalInstance* cmpSigInst,
                   netSignal->getComponentSignals()) {
            foreach (BI_FootprintPad* pad,
                     cmpSigInst->getRegisteredFootprintPads()) {
              if ((pad->isOnLayer(layer)) &&
                  (pad->getPosition() - pos).getLength() < Length(100)) {
                padMap.insert(
                    std::make_pair(pad->getDevice().getComponentInstanceUuid(),
                                   pad->getLibPadUuid()),
                    pad);
                return TraceAnchor::pad(
                    pad->getDevice().getComponentInstanceUuid(),
                    pad->getLibPadUuid());
              }
            }
          }
        }
        // Find or add net point.
        if (it == anchorMap.end()) {
          const Junction junction(Uuid::createRandom(), pos);
          splitter.addJunction(junction);
          it = anchorMap.insert(std::make_pair(&layer, pos),
                                TraceAnchor::junction(junction.getUuid()));
        }
        return *it;
      };
      foreach (const parseagle::Wire& eagleWire, eagleSignal.getWires()) {
        // Skip zero-length wires to avoid possible redundant wires.
        if (eagleWire.getP1() == eagleWire.getP2()) {
          continue;
        }
        // Skip "unrouted" wires (i.e. airwires) since they will be rebuilt.
        if (eagleWire.getLayer() == 19) {
          continue;
        }
        const Layer* layer =
            mapLayer(C::tryConvertBoardLayer(eagleWire.getLayer()));
        if ((!layer) || (!layer->isCopper())) {
          log.critical(QString("Skipped trace on invalid layer: %1")
                           .arg(eagleWire.getLayer()));
          continue;
        }
        const TraceAnchor startAnchor =
            getOrCreateAnchor(*layer, C::convertPoint(eagleWire.getP1()));
        const TraceAnchor endAnchor =
            getOrCreateAnchor(*layer, C::convertPoint(eagleWire.getP2()));
        if (startAnchor == endAnchor) {
          log.info("Attaching a trace to a pad removed a short trace segment.");
          continue;
        }
        if (eagleWire.getWireStyle() != parseagle::WireStyle::Continuous) {
          log.critical(
              tr("Dashed/dotted trace is not supported, converting to "
                 "continuous."));
        }
        if (eagleWire.getWireCap() != parseagle::WireCap::Round) {
          log.critical(
              tr("Flat trace end is not supported, converting to round."));
        }
        if (eagleWire.getCurve() != 0) {
          log.critical(
              tr("Curved trace is not supported, converting to straight."));
        }
        splitter.addTrace(
            Trace(Uuid::createRandom(), *layer,
                  PositiveLength(C::convertLength(eagleWire.getWidth())),
                  startAnchor, endAnchor));
      }

      // Determine segments and add them to the board.
      auto getAnchor = [&padMap, &viaMap,
                        &netPointMap](const TraceAnchor& anchor) {
        if (auto pad = anchor.tryGetPad()) {
          auto padPtr = padMap.value(std::make_pair(pad->device, pad->pad));
          if (padPtr) {
            return static_cast<BI_NetLineAnchor*>(padPtr);
          }
        } else if (auto via = anchor.tryGetVia()) {
          if (auto viaPtr = viaMap.value(*via)) {
            return static_cast<BI_NetLineAnchor*>(viaPtr);
          }
        } else if (auto junction = anchor.tryGetJunction()) {
          if (auto netPoint = netPointMap.value(*junction)) {
            return static_cast<BI_NetLineAnchor*>(netPoint);
          }
        }
        throw LogicError(__FILE__, __LINE__, "Unknown trace anchor.");
      };
      foreach (const auto& segment, splitter.split()) {
        BI_NetSegment* netSegment =
            new BI_NetSegment(*board, Uuid::createRandom(), netSignal);
        board->addNetSegment(*netSegment);
        QList<BI_Via*> vias;
        QList<BI_NetPoint*> netPoints;
        QList<BI_NetLine*> netLines;
        for (const Via& via : segment.vias) {
          BI_Via* v = new BI_Via(*netSegment, via);
          viaMap.insert(v->getUuid(), v);
          vias.append(v);
        }
        for (const Junction& junction : segment.junctions) {
          BI_NetPoint* np = new BI_NetPoint(*netSegment, junction.getUuid(),
                                            junction.getPosition());
          netPointMap.insert(np->getUuid(), np);
          netPoints.append(np);
        }
        for (const Trace& trace : segment.traces) {
          netLines.append(new BI_NetLine(*netSegment, trace.getUuid(),
                                         *getAnchor(trace.getStartPoint()),
                                         *getAnchor(trace.getEndPoint()),
                                         trace.getLayer(), trace.getWidth()));
        }
        netSegment->addElements(vias, netPoints, netLines);
      }
    } catch (const Exception& e) {
      log.critical(QString("Failed to import segment of net '%1': %2")
                       .arg(eagleSignal.getName(), e.getMsg()));
    }

    // Add planes and keepout zones.
    foreach (const parseagle::Polygon& eagleObj, eagleSignal.getPolygons()) {
      try {
        const Layer* layer =
            mapLayer(C::tryConvertBoardLayer(eagleObj.getLayer()));
        if ((!layer) || (!layer->isCopper())) {
          throw RuntimeError(__FILE__, __LINE__, "Plane not on copper layer.");
        }
        if (eagleObj.getPour() == parseagle::PolygonPour::Cutout) {
          const Path path = C::convertVertices(eagleObj.getVertices(), false);
          const Length lineWidth = C::convertLength(eagleObj.getWidth());
          foreach (const Path& outline,
                   C::convertBoardZoneOutline(path, lineWidth)) {
            BI_Zone* zone = new BI_Zone(
                *board,
                BoardZoneData(Uuid::createRandom(), {layer},
                              Zone::Rule::NoPlanes, outline, false));
            board->addZone(*zone);
          }
        } else {
          const Length isolate = C::convertLength(eagleObj.getIsolate());
          BI_Plane* obj =
              new BI_Plane(*board, Uuid::createRandom(), *layer, netSignal,
                           C::convertVertices(eagleObj.getVertices(), false));
          obj->setMinWidth(
              UnsignedLength(C::convertLength(eagleObj.getWidth())));
          obj->setMinClearance(
              (isolate > 0)
                  ? UnsignedLength(isolate)
                  : board->getDrcSettings().getMinCopperCopperClearance());
          obj->setConnectStyle(eagleObj.getThermals()
                                   ? BI_Plane::ConnectStyle::ThermalRelief
                                   : BI_Plane::ConnectStyle::Solid);
          // obj->setThermalGap();
          if (obj->getThermalSpokeWidth() < obj->getMinWidth()) {
            // Avoid possibly disappearing planes.
            obj->setThermalSpokeWidth(PositiveLength(*obj->getMinWidth()));
          }
          obj->setPriority(6 - eagleObj.getRank());  // EAGLE: 1..6
          obj->setKeepIslands(eagleObj.getOrphans());
          board->addPlane(*obj);
        }
      } catch (const Exception& e) {
        log.critical(QString("Skipped plane: %1").arg(e.getMsg()));
      }
    }
  }
}

bool EagleProjectImport::hasBuses(
    const parseagle::Schematic& schematic) const noexcept {
  int buses = 0;
  foreach (const parseagle::Sheet& sheet, schematic.getSheets()) {
    buses += sheet.getBuses().count();
  }
  return buses > 0;
}

std::optional<BoundedUnsignedRatio> EagleProjectImport::tryGetDrcRatio(
    const QString& nr, const QString& nmin, const QString& nmax) const {
  if (mBoard) {
    const auto pr = mBoard->getDesignRules().tryGetParam(nr);
    const auto pmin = mBoard->getDesignRules().tryGetParam(nmin);
    const auto pmax = mBoard->getDesignRules().tryGetParam(nmax);
    if (pr && pmin && pmax) {
      const auto vr = C::convertParamTo<UnsignedRatio>(*pr);
      const auto vmin = C::convertParamTo<UnsignedLength>(*pmin);
      const auto vmax = C::convertParamTo<UnsignedLength>(*pmax);
      // Note: Eagle allows to specify min>max so we have to correct this
      // case. It seems the min value is ignored then.
      return std::make_optional(
          BoundedUnsignedRatio(vr, std::min(vmin, vmax), vmax));
    }
  }
  return std::nullopt;
}

std::shared_ptr<const parseagle::Symbol> EagleProjectImport::getSymbol(
    const QString& libName, const QString& libUrn, const QString& name) const {
  const QStringList key{libName, libUrn, name};
  if (auto ptr = mSymbols.value(key)) {
    return ptr;
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("Symbol not found in embedded library: %1").arg(key.join("::")));
}

std::shared_ptr<const parseagle::Package> EagleProjectImport::getPackage(
    const QString& libName, const QString& libUrn, const QString& name) const {
  const QStringList key{libName, libUrn, name};
  if (auto ptr = mPackages.value(key)) {
    return ptr;
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("Package not found in embedded library: %1").arg(key.join("::")));
}

std::shared_ptr<const parseagle::DeviceSet> EagleProjectImport::getDeviceSet(
    const QString& libName, const QString& libUrn, const QString& name) const {
  const QStringList key{libName, libUrn, name};
  if (auto ptr = mDeviceSets.value(key)) {
    return ptr;
  }
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Device set not found in embedded library: %1")
                         .arg(key.join("::")));
}

const parseagle::Device& EagleProjectImport::getDevice(
    const parseagle::DeviceSet& devSet, const QString& name) const {
  foreach (const parseagle::Device& obj, devSet.getDevices()) {
    if (obj.getName() == name) {
      return obj;
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("Device not found in embedded library: %1").arg(name));
}

const parseagle::Technology* EagleProjectImport::tryGetTechnology(
    const parseagle::Device& dev, const QString& name) const {
  foreach (const parseagle::Technology& obj, dev.getTechnologies()) {
    if (obj.getName() == name) {
      return &obj;
    }
  }
  return nullptr;
}

const parseagle::Part& EagleProjectImport::getPart(const QString& name) const {
  foreach (const parseagle::Part& obj, mSchematic->getParts()) {
    if (obj.getName() == name) {
      return obj;
    }
  }
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Part not found: %1").arg(name));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb
