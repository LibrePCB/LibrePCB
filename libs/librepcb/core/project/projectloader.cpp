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
#include "projectloader.h"

#include "../application.h"
#include "../fileio/versionfile.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/pkg/package.h"
#include "../library/sym/symbol.h"
#include "../serialization/fileformatmigration.h"
#include "board/board.h"
#include "board/boarddesignrules.h"
#include "board/boardfabricationoutputsettings.h"
#include "board/boardlayerstack.h"
#include "board/drc/boarddesignrulechecksettings.h"
#include "board/items/bi_device.h"
#include "board/items/bi_footprintpad.h"
#include "board/items/bi_hole.h"
#include "board/items/bi_netline.h"
#include "board/items/bi_netpoint.h"
#include "board/items/bi_netsegment.h"
#include "board/items/bi_plane.h"
#include "board/items/bi_polygon.h"
#include "board/items/bi_stroketext.h"
#include "board/items/bi_via.h"
#include "circuit/circuit.h"
#include "circuit/componentinstance.h"
#include "circuit/componentsignalinstance.h"
#include "circuit/netclass.h"
#include "circuit/netsignal.h"
#include "erc/electricalrulecheck.h"
#include "project.h"
#include "projectlibrary.h"
#include "schematic/items/si_netlabel.h"
#include "schematic/items/si_netline.h"
#include "schematic/items/si_netpoint.h"
#include "schematic/items/si_netsegment.h"
#include "schematic/items/si_polygon.h"
#include "schematic/items/si_symbol.h"
#include "schematic/items/si_symbolpin.h"
#include "schematic/items/si_text.h"
#include "schematic/schematic.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectLoader::ProjectLoader(QObject* parent) noexcept : QObject(parent) {
}

ProjectLoader::~ProjectLoader() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<Project> ProjectLoader::open(
    std::unique_ptr<TransactionalDirectory> directory,
    const QString& filename) {
  Q_ASSERT(directory);
  mUpgradeMessages = tl::nullopt;

  QElapsedTimer timer;
  timer.start();
  const FilePath fp = directory->getAbsPath(filename);
  qDebug().nospace() << "Open project " << fp.toNative() << "...";

  // Check if the project file exists.
  if (!directory->fileExists(filename)) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("File does not exist: '%1'").arg(fp.toNative()));
  }

  // Read the file format version.
  if (!directory->fileExists(".librepcb-project")) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Directory does not contain a LibrePCB project: '%1'")
                           .arg(directory->getAbsPath().toNative()));
  }
  const Version fileFormat =
      VersionFile::fromByteArray(directory->read(".librepcb-project"))
          .getVersion();
  qDebug().noquote() << "Detected project file format:" << fileFormat.toStr();

  // Check file format version.
  if (fileFormat > qApp->getFileFormatVersion()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("This project was created with a newer application version.\n"
           "You need at least LibrePCB %1 to open it.\n\n%2")
            .arg(fileFormat.toPrettyStr(3))
            .arg(fp.toNative()));
  }

  // Upgrate file format, if needed.
  for (auto migration : FileFormatMigration::getMigrations(fileFormat)) {
    if (!mUpgradeMessages) {
      mUpgradeMessages = QList<FileFormatMigration::Message>();
    }
    qInfo().nospace().noquote()
        << "Project file format is outdated, upgrading from v"
        << migration->getFromVersion().toStr() << " to v"
        << migration->getToVersion().toStr() << "...";
    migration->upgradeProject(*directory, *mUpgradeMessages);
  }

  // Load project.
  std::unique_ptr<Project> p(new Project(std::move(directory), filename));
  loadMetadata(*p);
  loadSettings(*p);
  loadLibrary(*p);
  loadCircuit(*p);
  loadErc(*p);
  loadSchematics(*p);
  loadBoards(*p);

  // If the file format was migrated, clean up obsolete ERC messages.
  if (mUpgradeMessages) {
    qInfo() << "Running ERC to clean up obsolete message approvals...";
    ElectricalRuleCheck erc(*p);
    const RuleCheckMessageList msgs = erc.runChecks();
    const QSet<SExpression> approvals = RuleCheckMessage::getAllApprovals(msgs);
    p->setErcMessageApprovals(p->getErcMessageApprovals() & approvals);
  }

  // Done!
  qDebug() << "Successfully opened project in" << timer.elapsed() << "ms.";
  return p;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectLoader::loadMetadata(Project& p) {
  qDebug() << "Load project metadata...";
  const QString fp = "project/metadata.lp";
  SExpression root = SExpression::parse(p.getDirectory().read(fp),
                                        p.getDirectory().getAbsPath(fp));

  p.setUuid(deserialize<Uuid>(root.getChild("@0")));
  p.setName(deserialize<ElementName>(root.getChild("name/@0")));
  p.setAuthor(root.getChild("author/@0").getValue());
  p.setVersion(root.getChild("version/@0").getValue());
  p.setCreated(deserialize<QDateTime>(root.getChild("created/@0")));
  p.setAttributes(AttributeList(root));

  qDebug() << "Successfully loaded project metadata.";
}

void ProjectLoader::loadSettings(Project& p) {
  qDebug() << "Load project settings...";
  const QString fp = "project/settings.lp";
  const SExpression root = SExpression::parse(p.getDirectory().read(fp),
                                              p.getDirectory().getAbsPath(fp));

  {
    QStringList l;
    foreach (const SExpression* node,
             root.getChild("library_locale_order").getChildren("locale")) {
      l.append(node->getChild("@0").getValue());
    }
    p.setLocaleOrder(l);
  }

  {
    QStringList l;
    foreach (const SExpression* node,
             root.getChild("library_norm_order").getChildren("norm")) {
      l.append(node->getChild("@0").getValue());
    }
    p.setNormOrder(l);
  }

  {
    QStringList l;
    foreach (const SExpression* node,
             root.getChild("custom_bom_attributes").getChildren("attribute")) {
      l.append(node->getChild("@0").getValue());
    }
    p.setCustomBomAttributes(l);
  }

  qDebug() << "Successfully loaded project settings.";
}

void ProjectLoader::loadLibrary(Project& p) {
  qDebug() << "Load project library...";

  loadLibraryElements<Symbol>(p, "sym", "symbols", &ProjectLibrary::addSymbol);
  loadLibraryElements<Package>(p, "pkg", "packages",
                               &ProjectLibrary::addPackage);
  loadLibraryElements<Component>(p, "cmp", "components",
                                 &ProjectLibrary::addComponent);
  loadLibraryElements<Device>(p, "dev", "devices", &ProjectLibrary::addDevice);

  qDebug() << "Successfully loaded project library.";
}

template <typename ElementType>
void ProjectLoader::loadLibraryElements(
    Project& p, const QString& dirname, const QString& type,
    void (ProjectLibrary::*addFunction)(ElementType&)) {
  // Search all subdirectories which have a valid UUID as directory name.
  int count = 0;
  foreach (const QString& sub, p.getLibrary().getDirectory().getDirs(dirname)) {
    std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(
        p.getLibrary().getDirectory(), dirname % "/" % sub));

    // Check if directory is a valid library element.
    if (!LibraryBaseElement::isValidElementDirectory<ElementType>(*dir, "")) {
      qWarning() << "Invalid directory in project library, ignoring it:"
                 << dir->getAbsPath().toNative();
      continue;
    }

    // Load the library element.
    ElementType* element =
        ElementType::open(std::move(dir)).release();  // can throw
    (p.getLibrary().*addFunction)(*element);
    ++count;
  }

  qDebug().nospace().noquote()
      << "Successfully loaded " << count << " " << type << ".";
}

void ProjectLoader::loadCircuit(Project& p) {
  qDebug() << "Load circuit...";
  const QString fp = "circuit/circuit.lp";
  SExpression root = SExpression::parse(p.getDirectory().read(fp),
                                        p.getDirectory().getAbsPath(fp));

  // Load net classes.
  foreach (const SExpression* node, root.getChildren("netclass")) {
    NetClass* netclass =
        new NetClass(p.getCircuit(), deserialize<Uuid>(node->getChild("@0")),
                     deserialize<ElementName>(node->getChild("name/@0")));
    p.getCircuit().addNetClass(*netclass);
  }

  // Load net signals.
  foreach (const SExpression* node, root.getChildren("net")) {
    const Uuid netclassUuid = deserialize<Uuid>(node->getChild("netclass/@0"));
    NetClass* netclass = p.getCircuit().getNetClasses().value(netclassUuid);
    if (!netclass) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Inexistent net class: '%1'").arg(netclassUuid.toStr()));
    }
    NetSignal* netsignal = new NetSignal(
        p.getCircuit(), deserialize<Uuid>(node->getChild("@0")), *netclass,
        deserialize<CircuitIdentifier>(node->getChild("name/@0")),
        deserialize<bool>(node->getChild("auto/@0")));
    p.getCircuit().addNetSignal(*netsignal);
  }

  // Load component instances.
  foreach (const SExpression* node, root.getChildren("component")) {
    const Uuid cmpUuid = deserialize<Uuid>(node->getChild("lib_component/@0"));
    const Component* libCmp = p.getLibrary().getComponent(cmpUuid);
    if (!libCmp) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("The component '%1' does not exist in the project's library.")
              .arg(cmpUuid.toStr()));
    }
    ComponentInstance* cmp = new ComponentInstance(
        p.getCircuit(), deserialize<Uuid>(node->getChild("@0")), *libCmp,
        deserialize<Uuid>(node->getChild("lib_variant/@0")),
        deserialize<CircuitIdentifier>(node->getChild("name/@0")),
        deserialize<tl::optional<Uuid>>(node->getChild("lib_device/@0")));
    cmp->setValue(node->getChild("value/@0").getValue());
    cmp->setAttributes(AttributeList(*node));
    p.getCircuit().addComponentInstance(*cmp);

    QSet<Uuid> loadedSignals;
    foreach (const SExpression* child, node->getChildren("signal")) {
      const Uuid cmpSigUuid = deserialize<Uuid>(child->getChild("@0"));
      ComponentSignalInstance* cmpSig = cmp->getSignalInstance(cmpSigUuid);
      if (!cmpSig) {
        throw RuntimeError(__FILE__, __LINE__,
                           QString("Inexistent component signal: '%1'")
                               .arg(cmpSigUuid.toStr()));
      }
      if (loadedSignals.contains(cmpSigUuid)) {
        throw RuntimeError(__FILE__, __LINE__,
                           QString("The signal '%1' is defined multiple times.")
                               .arg(cmpSigUuid.toStr()));
      }
      loadedSignals.insert(cmpSigUuid);
      if (const tl::optional<Uuid> netSignalUuid =
              deserialize<tl::optional<Uuid>>(child->getChild("net/@0"))) {
        NetSignal* netSignal =
            p.getCircuit().getNetSignals().value(*netSignalUuid);
        if (!netSignal) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Inexistent net signal: '%1'")
                                 .arg(netSignalUuid->toStr()));
        }
        cmpSig->setNetSignal(netSignal);
      }
    }
    if (loadedSignals.count() != cmp->getSignals().count()) {
      qCritical() << "Signal count mismatch:" << loadedSignals.count()
                  << "!=" << cmp->getSignals().count();
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("The signal count of the component instance '%1' does "
                  "not match with the signal count of the component '%2'.")
              .arg(cmp->getUuid().toStr())
              .arg(libCmp->getUuid().toStr()));
    }
  }

  qDebug() << "Successfully loaded circuit.";
}

void ProjectLoader::loadErc(Project& p) {
  qDebug() << "Load ERC approvals...";
  const QString fp = "circuit/erc.lp";
  const SExpression root = SExpression::parse(p.getDirectory().read(fp),
                                              p.getDirectory().getAbsPath(fp));

  // Load approvals.
  QSet<SExpression> approvals;
  foreach (const SExpression* node, root.getChildren("approved")) {
    approvals.insert(*node);
  }
  p.setErcMessageApprovals(approvals);

  qDebug() << "Successfully loaded ERC approvals.";
}

void ProjectLoader::loadSchematics(Project& p) {
  qDebug() << "Load schematics...";
  const QString fp = "schematics/schematics.lp";
  const SExpression indexRoot = SExpression::parse(
      p.getDirectory().read(fp), p.getDirectory().getAbsPath(fp));
  foreach (const SExpression* indexNode, indexRoot.getChildren("schematic")) {
    loadSchematic(p, indexNode->getChild("@0").getValue());
  }
  qDebug() << "Successfully loaded" << p.getSchematics().count()
           << "schematics.";
}

void ProjectLoader::loadSchematic(Project& p, const QString& relativeFilePath) {
  const FilePath fp = FilePath::fromRelative(p.getPath(), relativeFilePath);
  std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(
      p.getDirectory(), fp.getParentDir().toRelative(p.getPath())));
  const SExpression root = SExpression::parse(dir->read(fp.getFilename()), fp);

  Schematic* schematic =
      new Schematic(p, std::move(dir), fp.getParentDir().getFilename(),
                    deserialize<Uuid>(root.getChild("@0")),
                    deserialize<ElementName>(root.getChild("name/@0")));
  schematic->setGridInterval(
      deserialize<PositiveLength>(root.getChild("grid/interval/@0")));
  schematic->setGridUnit(
      deserialize<LengthUnit>(root.getChild("grid/unit/@0")));
  p.addSchematic(*schematic);

  foreach (const SExpression* node, root.getChildren("symbol")) {
    loadSchematicSymbol(*schematic, *node);
  }
  foreach (const SExpression* node, root.getChildren("netsegment")) {
    loadSchematicNetSegment(*schematic, *node);
  }
  foreach (const SExpression* node, root.getChildren("polygon")) {
    SI_Polygon* polygon = new SI_Polygon(*schematic, Polygon(*node));
    schematic->addPolygon(*polygon);
  }
  foreach (const SExpression* node, root.getChildren("text")) {
    SI_Text* text = new SI_Text(*schematic, Text(*node));
    schematic->addText(*text);
  }
}

void ProjectLoader::loadSchematicSymbol(Schematic& s, const SExpression& node) {
  const Uuid cmpUuid = deserialize<Uuid>(node.getChild("component/@0"));
  ComponentInstance* cmp =
      s.getProject().getCircuit().getComponentInstanceByUuid(cmpUuid);
  if (!cmp) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The component '%1' does not exist in the circuit.")
            .arg(cmpUuid.toStr()));
  }
  SI_Symbol* symbol =
      new SI_Symbol(s, deserialize<Uuid>(node.getChild("@0")), *cmp,
                    deserialize<Uuid>(node.getChild("lib_gate/@0")),
                    Point(node.getChild("position")),
                    deserialize<Angle>(node.getChild("rotation/@0")),
                    deserialize<bool>(node.getChild("mirror/@0")), false);
  foreach (const SExpression* child, node.getChildren("text")) {
    symbol->addText(*new SI_Text(s, Text(*child)));
  }
  s.addSymbol(*symbol);
}

void ProjectLoader::loadSchematicNetSegment(Schematic& s,
                                            const SExpression& node) {
  const Uuid netSignalUuid = deserialize<Uuid>(node.getChild("net/@0"));
  NetSignal* netSignal =
      s.getProject().getCircuit().getNetSignals().value(netSignalUuid);
  if (!netSignal) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Inexistent net signal: '%1'").arg(netSignalUuid.toStr()));
  }
  SI_NetSegment* netSegment =
      new SI_NetSegment(s, deserialize<Uuid>(node.getChild("@0")), *netSignal);
  s.addNetSegment(*netSegment);

  // Load net points.
  QList<SI_NetPoint*> netPoints;
  foreach (const SExpression* child, node.getChildren("junction")) {
    SI_NetPoint* netpoint =
        new SI_NetPoint(*netSegment, deserialize<Uuid>(child->getChild("@0")),
                        Point(child->getChild("position")));
    netPoints.append(netpoint);
  }

  // Load net lines.
  QList<SI_NetLine*> netLines;
  foreach (const SExpression* child, node.getChildren("line")) {
    auto parseAnchor = [&s, &netPoints](const SExpression& aNode) {
      SI_NetLineAnchor* anchor = nullptr;
      if (const SExpression* junctionNode = aNode.tryGetChild("junction")) {
        const Uuid netPointUuid =
            deserialize<Uuid>(junctionNode->getChild("@0"));
        foreach (SI_NetPoint* netPoint, netPoints) {
          if (netPoint->getUuid() == netPointUuid) {
            anchor = netPoint;
            break;
          }
        }
        if (!anchor) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("Net point '%1' does not exist in schematic.")
                  .arg(netPointUuid.toStr()));
        }
      } else {
        const Uuid symbolUuid = deserialize<Uuid>(aNode.getChild("symbol/@0"));
        SI_Symbol* symbol = s.getSymbols().value(symbolUuid);
        if (!symbol) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Symbol '%1' does not exist in schematic.")
                                 .arg(symbolUuid.toStr()));
        }
        const Uuid pinUuid = deserialize<Uuid>(aNode.getChild("pin/@0"));
        anchor = symbol->getPin(pinUuid);
        if (!anchor) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("Symbol pin '%1' does not exist in schematic.")
                  .arg(pinUuid.toStr()));
        }
      }
      return anchor;
    };
    SI_NetLine* netLine = new SI_NetLine(
        *netSegment, deserialize<Uuid>(child->getChild("@0")),
        *parseAnchor(child->getChild("from")),
        *parseAnchor(child->getChild("to")),
        deserialize<UnsignedLength>(child->getChild("width/@0")));
    netLines.append(netLine);
  }

  // Add net points & net lines.
  netSegment->addNetPointsAndNetLines(netPoints, netLines);

  // Load net labels.
  foreach (const SExpression* child, node.getChildren("label")) {
    SI_NetLabel* netLabel = new SI_NetLabel(*netSegment, NetLabel(*child));
    netSegment->addNetLabel(*netLabel);
  }
}

void ProjectLoader::loadBoards(Project& p) {
  qDebug() << "Load boards...";
  const QString fp = "boards/boards.lp";
  const SExpression indexRoot = SExpression::parse(
      p.getDirectory().read(fp), p.getDirectory().getAbsPath(fp));
  foreach (const SExpression* node, indexRoot.getChildren("board")) {
    loadBoard(p, node->getChild("@0").getValue());
  }
  qDebug() << "Successfully loaded" << p.getBoards().count() << "boards.";
}

void ProjectLoader::loadBoard(Project& p, const QString& relativeFilePath) {
  const FilePath fp = FilePath::fromRelative(p.getPath(), relativeFilePath);
  std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(
      p.getDirectory(), fp.getParentDir().toRelative(p.getPath())));
  const SExpression root = SExpression::parse(dir->read(fp.getFilename()), fp);

  Board* board = new Board(p, std::move(dir), fp.getParentDir().getFilename(),
                           deserialize<Uuid>(root.getChild("@0")),
                           deserialize<ElementName>(root.getChild("name/@0")));
  board->setGridInterval(
      deserialize<PositiveLength>(root.getChild("grid/interval/@0")));
  board->setGridUnit(deserialize<LengthUnit>(root.getChild("grid/unit/@0")));
  board->setDefaultFontName(root.getChild("default_font/@0").getValue());
  board->getLayerStack().setInnerLayerCount(
      deserialize<uint>(root.getChild("layers/inner/@0")));
  board->setDesignRules(BoardDesignRules(root.getChild("design_rules")));
  {
    const SExpression& node = root.getChild("design_rule_check");
    const Version approvalsVersion =
        deserialize<Version>(node.getChild("approvals_version/@0"));
    QSet<SExpression> approvals;
    foreach (const SExpression* child, node.getChildren("approved")) {
      approvals.insert(*child);
    }
    board->setDrcSettings(BoardDesignRuleCheckSettings(node));
    board->loadDrcMessageApprovals(approvalsVersion, approvals);
  }
  board->getFabricationOutputSettings() = BoardFabricationOutputSettings(
      root.getChild("fabrication_output_settings"));
  p.addBoard(*board);

  foreach (const SExpression* node, root.getChildren("device")) {
    loadBoardDeviceInstance(*board, *node);
  }
  foreach (const SExpression* node, root.getChildren("netsegment")) {
    loadBoardNetSegment(*board, *node);
  }
  foreach (const SExpression* node, root.getChildren("plane")) {
    loadBoardPlane(*board, *node);
  }
  foreach (const SExpression* node, root.getChildren("polygon")) {
    BI_Polygon* polygon = new BI_Polygon(*board, Polygon(*node));
    board->addPolygon(*polygon);
  }
  foreach (const SExpression* node, root.getChildren("stroke_text")) {
    BI_StrokeText* text = new BI_StrokeText(*board, StrokeText(*node));
    board->addStrokeText(*text);
  }
  foreach (const SExpression* node, root.getChildren("hole")) {
    BI_Hole* hole = new BI_Hole(*board, Hole(*node));
    board->addHole(*hole);
  }

  // Rebuild all planes.
  board->rebuildAllPlanes();

  // Load user settings.
  loadBoardUserSettings(*board);
}

void ProjectLoader::loadBoardDeviceInstance(Board& b, const SExpression& node) {
  const Uuid cmpUuid = deserialize<Uuid>(node.getChild("@0"));
  ComponentInstance* cmp =
      b.getProject().getCircuit().getComponentInstanceByUuid(cmpUuid);
  if (!cmp) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The component instace '%1' does not exist in the circuit.")
            .arg(cmpUuid.toStr()));
  }
  BI_Device* device =
      new BI_Device(b, *cmp, deserialize<Uuid>(node.getChild("lib_device/@0")),
                    deserialize<Uuid>(node.getChild("lib_footprint/@0")),
                    Point(node.getChild("position")),
                    deserialize<Angle>(node.getChild("rotation/@0")),
                    deserialize<bool>(node.getChild("mirror/@0")), false);
  device->setAttributes(AttributeList(node));
  foreach (const SExpression* child, node.getChildren("stroke_text")) {
    device->addStrokeText(*new BI_StrokeText(b, StrokeText(*child)));
  }
  b.addDeviceInstance(*device);
}

void ProjectLoader::loadBoardNetSegment(Board& b, const SExpression& node) {
  const tl::optional<Uuid> netSignalUuid =
      deserialize<tl::optional<Uuid>>(node.getChild("net/@0"));
  NetSignal* netSignal = netSignalUuid
      ? b.getProject().getCircuit().getNetSignals().value(*netSignalUuid)
      : nullptr;
  if (netSignalUuid && (!netSignal)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Inexistent net signal: '%1'").arg(netSignalUuid->toStr()));
  }
  BI_NetSegment* netSegment =
      new BI_NetSegment(b, deserialize<Uuid>(node.getChild("@0")), netSignal);
  b.addNetSegment(*netSegment);

  // Load vias.
  QList<BI_Via*> vias;
  foreach (const SExpression* child, node.getChildren("via")) {
    BI_Via* via = new BI_Via(*netSegment, Via(*child));
    vias.append(via);
  }

  // Load net points.
  QList<BI_NetPoint*> netPoints;
  foreach (const SExpression* child, node.getChildren("junction")) {
    BI_NetPoint* netPoint =
        new BI_NetPoint(*netSegment, deserialize<Uuid>(child->getChild("@0")),
                        Point(child->getChild("position")));
    netPoints.append(netPoint);
  }

  // Load net lines.
  QList<BI_NetLine*> netLines;
  foreach (const SExpression* child, node.getChildren("trace")) {
    auto parseAnchor = [&b, &vias, &netPoints](const SExpression& aNode) {
      BI_NetLineAnchor* anchor = nullptr;
      if (const SExpression* junctionNode = aNode.tryGetChild("junction")) {
        const Uuid netPointUuid =
            deserialize<Uuid>(junctionNode->getChild("@0"));
        foreach (BI_NetPoint* netPoint, netPoints) {
          if (netPoint->getUuid() == netPointUuid) {
            anchor = netPoint;
            break;
          }
        }
        if (!anchor) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("Net point '%1' does not exist in schematic.")
                  .arg(netPointUuid.toStr()));
        }
      } else if (const SExpression* viaNode = aNode.tryGetChild("via")) {
        const Uuid viaUuid = deserialize<Uuid>(viaNode->getChild("@0"));
        foreach (BI_Via* via, vias) {
          if (via->getUuid() == viaUuid) {
            anchor = via;
            break;
          }
        }
        if (!anchor) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Via '%1' does not exist in board.")
                                 .arg(viaUuid.toStr()));
        }
      } else {
        const Uuid deviceUuid = deserialize<Uuid>(aNode.getChild("device/@0"));
        BI_Device* device = b.getDeviceInstanceByComponentUuid(deviceUuid);
        if (!device) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("Device instance '%1' does not exist in board.")
                  .arg(deviceUuid.toStr()));
        }
        const Uuid padUuid = deserialize<Uuid>(aNode.getChild("pad/@0"));
        anchor = device->getPad(padUuid);
        if (!anchor) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("Footprint pad '%1' does not exist in board.")
                  .arg(padUuid.toStr()));
        }
      }
      return anchor;
    };
    const QString layerName = child->getChild("layer/@0").getValue();
    GraphicsLayer* layer = b.getLayerStack().getLayer(layerName);
    if (!layer) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid board layer: '%1'").arg(layerName));
    }
    BI_NetLine* netLine = new BI_NetLine(
        *netSegment, deserialize<Uuid>(child->getChild("@0")),
        *parseAnchor(child->getChild("from")),
        *parseAnchor(child->getChild("to")), *layer,
        deserialize<PositiveLength>(child->getChild("width/@0")));
    netLines.append(netLine);
  }

  // Add vias, net points & net lines.
  netSegment->addElements(vias, netPoints, netLines);
}

void ProjectLoader::loadBoardPlane(Board& b, const SExpression& node) {
  const Uuid netSignalUuid = deserialize<Uuid>(node.getChild("net/@0"));
  NetSignal* netSignal =
      b.getProject().getCircuit().getNetSignals().value(netSignalUuid);
  if (!netSignal) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Inexistent net signal: '%1'").arg(netSignalUuid.toStr()));
  }
  BI_Plane* plane =
      new BI_Plane(b, deserialize<Uuid>(node.getChild("@0")),
                   deserialize<GraphicsLayerName>(node.getChild("layer/@0")),
                   *netSignal, Path(node));
  plane->setMinWidth(
      deserialize<UnsignedLength>(node.getChild("min_width/@0")));
  plane->setMinClearance(
      deserialize<UnsignedLength>(node.getChild("min_clearance/@0")));
  plane->setKeepOrphans(deserialize<bool>(node.getChild("keep_orphans/@0")));
  plane->setPriority(deserialize<int>(node.getChild("priority/@0")));
  plane->setConnectStyle(
      deserialize<BI_Plane::ConnectStyle>(node.getChild("connect_style/@0")));
  b.addPlane(*plane);
}

void ProjectLoader::loadBoardUserSettings(Board& b) {
  try {
    const QString fp = "settings.user.lp";
    const SExpression root = SExpression::parse(
        b.getDirectory().read(fp), b.getDirectory().getAbsPath(fp));

    // Layers.
    for (const SExpression* node : root.getChildren("layer")) {
      const QString name = node->getChild("@0").getValue();
      if (GraphicsLayer* layer = b.getLayerStack().getLayer(name)) {
        layer->setVisible(deserialize<bool>(node->getChild("visible/@0")));
      } else {
        qWarning()
            << "Layer" << name
            << "doesn't exist, could not restore its appearance settings.";
      }
    }

    // Planes visibility.
    foreach (const SExpression* node, root.getChildren("plane")) {
      const Uuid uuid = deserialize<Uuid>(node->getChild("@0"));
      if (BI_Plane* plane = b.getPlanes().value(uuid)) {
        plane->setVisible(deserialize<bool>(node->getChild("visible/@0")));
      } else {
        qWarning() << "Plane" << uuid.toStr()
                   << "doesn't exist, could not restore its visibility.";
      }
    }
  } catch (const Exception&) {
    // Project user settings are normally not put under version control and
    // thus the likelyhood of parse errors is higher (e.g. when switching to
    // an older, now incompatible revision). To avoid frustration, we just
    // ignore these errors and load the default settings instead...
    qCritical() << "Could not load board user settings, defaults will be "
                   "used instead.";
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
