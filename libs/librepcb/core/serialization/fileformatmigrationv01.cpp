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
#include "fileformatmigrationv01.h"

#include "../fileio/transactionaldirectory.h"
#include "../fileio/versionfile.h"
#include "../geometry/path.h"
#include "../types/alignment.h"
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/point.h"
#include "../types/simplestring.h"
#include "../types/uuid.h"
#include "sexpression.h"

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileFormatMigrationV01::FileFormatMigrationV01(QObject* parent) noexcept
  : FileFormatMigration(Version::fromString("0.1"), Version::fromString("0.2"),
                        parent) {
}

FileFormatMigrationV01::~FileFormatMigrationV01() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FileFormatMigrationV01::upgradeComponentCategory(
    TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-cmpcat");
}

void FileFormatMigrationV01::upgradePackageCategory(
    TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-pkgcat");
}

void FileFormatMigrationV01::upgradeSymbol(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-sym");

  // Content File.
  {
    const QString fp = "symbol.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root.appendChild("generated_by", QString());

    // Various strings.
    upgradeStrings(root);

    // Layers.
    upgradeLayers(root);

    // Pins.
    upgradeInversionCharacters(root, "pin", "name/@0");
    for (SExpression* pinNode : root.getChildren("pin")) {
      const UnsignedLength length =
          deserialize<UnsignedLength>(pinNode->getChild("length/@0"));
      const Point namePos(length + Length(1270000), 0);
      const Alignment nameAlign(HAlign::left(), VAlign::center());
      namePos.serialize(pinNode->appendList("name_position"));
      pinNode->appendChild("name_rotation", Angle::deg0());
      pinNode->appendChild("name_height", PositiveLength(2500000));
      nameAlign.serialize(pinNode->appendList("name_align"));
    }

    dir.write(fp, root.toByteArray());
  }
}

void FileFormatMigrationV01::upgradePackage(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-pkg");

  // Content File.
  {
    const QString fp = "package.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root.appendChild("generated_by", QString());

    // Various strings.
    upgradeStrings(root);

    // Layers.
    upgradeLayers(root);

    // Assembly type.
    root.appendChild("assembly_type", SExpression::createToken("auto"));

    // Footprints.
    for (SExpression* fptNode : root.getChildren("footprint")) {
      // Add 3D model position.
      SExpression& modelPosition = fptNode->appendList("3d_position");
      modelPosition.appendChild(SExpression::createToken("0.0"));
      modelPosition.appendChild(SExpression::createToken("0.0"));
      modelPosition.appendChild(SExpression::createToken("0.0"));

      // Add 3D model rotation.
      SExpression& modelRotation = fptNode->appendList("3d_rotation");
      modelRotation.appendChild(SExpression::createToken("0.0"));
      modelRotation.appendChild(SExpression::createToken("0.0"));
      modelRotation.appendChild(SExpression::createToken("0.0"));

      // Pads.
      for (SExpression* padNode : fptNode->getChildren("pad")) {
        // In the file format 0.1, footprint pads did not have their own UUID,
        // but only the UUID of the package pad they were connected to. To get
        // a deterministic UUID when upgrading a v0.1 footprint pad to v0.2,
        // we simply use the package pad UUID as the footprint pad UUID too.
        // See https://github.com/LibrePCB/LibrePCB/issues/445
        const Uuid uuid = deserialize<Uuid>(padNode->getChild("@0"));
        padNode->appendChild("package_pad", uuid);

        // Convert shape & corner radius.
        SExpression& padShape = padNode->getChild("shape/@0");
        const bool isRoundShape = (padShape.getValue() == "round");
        const bool isRectShape = (padShape.getValue() == "rect");
        padNode->appendChild(
            "radius", SExpression::createToken(isRoundShape ? "1.0" : "0.0"));
        if (isRoundShape || isRectShape) {
          padShape = SExpression::createToken("roundrect");
        }

        // Convert holes.
        // Note: In the Gerber export, drills on SMT pads were ignored thus
        // we delete such drills now to keep the same behavior.
        // To get a deterministic UUID, the pad's UUID is reused for the hole.
        SExpression& boardSideNode = padNode->getChild("side/@0");
        const UnsignedLength drill =
            deserialize<UnsignedLength>(padNode->getChild("drill/@0"));
        if ((boardSideNode.getValue() == "tht") && (drill > 0)) {
          SExpression& holeNode = padNode->appendList("hole");
          holeNode.appendChild(uuid);
          holeNode.appendChild("diameter", drill);
          SExpression& vertexNode = holeNode.appendList("vertex");
          SExpression& positionNode = vertexNode.appendList("position");
          positionNode.appendChild(Length(0));  // X
          positionNode.appendChild(Length(0));  // Y
          vertexNode.appendChild("angle", Angle::deg0());
        }
        if (boardSideNode.getValue() == "tht") {
          // THT is no longer a valid value. Since footprints are always drawn
          // from the top view, it should be safe to set it to "top" now.
          boardSideNode = SExpression::createToken("top");
        }

        // Add mask configs.
        padNode->appendChild("stop_mask", SExpression::createToken("auto"));
        padNode->appendChild(
            "solder_paste",
            SExpression::createToken((drill > 0) ? "off" : "auto"));

        // Add function.
        padNode->appendChild("function",
                             SExpression::createToken("unspecified"));

        // Add copper clearance.
        padNode->appendChild("clearance", SExpression::createToken("0.0"));
      }

      // Polygons.
      for (SExpression* polygonNode : fptNode->getChildren("polygon")) {
        if (polygonNode->getChild("layer/@0")
                .getValue()
                .endsWith("_courtyard")) {
          SExpression& widthNode = polygonNode->getChild("width/@0");
          widthNode.setValue("0.0");
        }
      }

      // Circles.
      for (SExpression* circleNode : fptNode->getChildren("circle")) {
        if (circleNode->getChild("layer/@0")
                .getValue()
                .endsWith("_courtyard")) {
          SExpression& widthNode = circleNode->getChild("width/@0");
          widthNode.setValue("0.0");
        }
      }

      // Stroke texts.
      for (SExpression* txtNode : fptNode->getChildren("stroke_text")) {
        if (deserialize<bool>(txtNode->getChild("mirror/@0"))) {
          SExpression& rotNode = txtNode->getChild("rotation/@0");
          rotNode = serialize(-deserialize<Angle>(rotNode));
        }
      }

      // Holes.
      upgradeHoles(*fptNode, false);

      // Move cutouts from the board outlines layer to the new cutouts layer.
      upgradeCutouts(*fptNode, nullptr);
    }

    dir.write(fp, root.toByteArray());
  }
}

void FileFormatMigrationV01::upgradeComponent(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-cmp");

  // Content File.
  {
    const QString fp = "component.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root.appendChild("generated_by", QString());

    // Signals.
    upgradeInversionCharacters(root, "signal", "name/@0");

    // Various strings.
    upgradeStrings(root);

    dir.write(fp, root.toByteArray());
  }
}

void FileFormatMigrationV01::upgradeDevice(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-dev");

  // Content File.
  {
    const QString fp = "device.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root.appendChild("generated_by", QString());

    // Various strings.
    upgradeStrings(root);

    dir.write(fp, root.toByteArray());
  }
}

void FileFormatMigrationV01::upgradeLibrary(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-lib");

  // Content File.
  {
    const QString fp = "library.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root.appendChild("manufacturer", QString());
    dir.write(fp, root.toByteArray());
  }
}

void FileFormatMigrationV01::upgradeProject(TransactionalDirectory& dir,
                                            QList<Message>& messages) {
  // ATTENTION: Do not actually perform any upgrade in this method! Instead,
  // just call virtual protected methods which do the upgrade. This allows
  // FileFormatMigrationUnstable to override them with partial upgrades.

  ProjectContext context;

  // Version File.
  upgradeVersionFile(dir, ".librepcb-project");

  // Symbols.
  foreach (const QString& dirName, dir.getDirs("library/sym")) {
    TransactionalDirectory subDir(dir, "library/sym/" % dirName);
    if (subDir.fileExists(".librepcb-sym")) {
      const QString fp = "symbol.lp";
      SExpression root =
          SExpression::parse(subDir.read(fp), subDir.getAbsPath(fp));
      const Uuid uuid = deserialize<Uuid>(root.getChild("@0"));
      Symbol sym;

      // Texts.
      for (SExpression* textNode : root.getChildren("text")) {
        sym.texts.append(Text{
            deserialize<Uuid>(textNode->getChild("@0")),
            textNode->getChild("layer/@0").getValue(),
            textNode->getChild("value/@0").getValue(),
            Point(textNode->getChild("position")),
            deserialize<Angle>(textNode->getChild("rotation/@0")),
            deserialize<PositiveLength>(textNode->getChild("height/@0")),
            Alignment(textNode->getChild("align")),
        });
      }

      context.symbols.insert(uuid, sym);

      upgradeSymbol(subDir);
    }
  }

  // Packages.
  foreach (const QString& dirName, dir.getDirs("library/pkg")) {
    TransactionalDirectory subDir(dir, "library/pkg/" % dirName);
    if (subDir.fileExists(".librepcb-pkg")) {
      const QString fp = "package.lp";
      SExpression root =
          SExpression::parse(subDir.read(fp), subDir.getAbsPath(fp));

      // Footprints.
      for (SExpression* fptNode : root.getChildren("footprint")) {
        context.holesCount += fptNode->getChildren("hole").count();
        foreach (
            const SExpression* geometryNode,
            fptNode->getChildren("polygon") + fptNode->getChildren("circle")) {
          if (geometryNode->getChild("layer/@0").getValue() == "brd_outlines") {
            ++context.footprintBoardOutlinesObjectCount;
          }
        }
      }

      upgradePackage(subDir);
    }
  }

  // Components.
  foreach (const QString& dirName, dir.getDirs("library/cmp")) {
    TransactionalDirectory subDir(dir, "library/cmp/" % dirName);
    if (subDir.fileExists(".librepcb-cmp")) {
      const QString fp = "component.lp";
      SExpression root =
          SExpression::parse(subDir.read(fp), subDir.getAbsPath(fp));
      const Uuid uuid = deserialize<Uuid>(root.getChild("@0"));
      Component cmp;
      cmp.schematicOnly = deserialize<bool>(root.getChild("schematic_only/@0"));

      // Symbol variants.
      for (SExpression* varNode : root.getChildren("variant")) {
        ComponentSymbolVariant symbVar{
            deserialize<Uuid>(varNode->getChild("@0")),
            {},
        };

        // Gates.
        for (SExpression* gateNode : varNode->getChildren("gate")) {
          symbVar.gates.append(Gate{
              deserialize<Uuid>(gateNode->getChild("@0")),
              deserialize<Uuid>(gateNode->getChild("symbol/@0")),
          });
        }

        cmp.symbolVariants.append(symbVar);
      }

      context.components.insert(uuid, cmp);

      upgradeComponent(subDir);
    }
  }

  // Devices.
  foreach (const QString& dirName, dir.getDirs("library/dev")) {
    TransactionalDirectory subDir(dir, "library/dev/" % dirName);
    if (subDir.fileExists(".librepcb-dev")) {
      upgradeDevice(subDir);
    }
  }

  // Scan boards.
  foreach (const QString& dirName, dir.getDirs("boards")) {
    QString fp = "boards/" % dirName % "/board.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
      foreach (const SExpression* devNode, root.getChildren("device")) {
        const Uuid cmpUuid = deserialize<Uuid>(devNode->getChild("@0"));
        const Uuid libDevUuid =
            deserialize<Uuid>(devNode->getChild("lib_device/@0"));
        context.devicesUsedInBoards[cmpUuid].insert(libDevUuid);
      }
    }
  }

  // Metadata.
  {
    const QString fp = "project/metadata.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    context.projectUuid = root.getChild("@0").getValue();
  }

  // Settings.
  {
    const QString fp = "project/settings.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeSettings(root);
    dir.write(fp, root.toByteArray());
  }

  // Circuit.
  {
    const QString fp = "circuit/circuit.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeCircuit(root, context);
    dir.write(fp, root.toByteArray());

    // Component instances.
    for (SExpression* cmpNode : root.getChildren("component")) {
      const Uuid uuid = deserialize<Uuid>(cmpNode->getChild("@0"));
      ComponentInstance cmpInst{
          deserialize<Uuid>(cmpNode->getChild("lib_component/@0")),
          deserialize<Uuid>(cmpNode->getChild("lib_variant/@0")),
      };
      context.componentInstances.insert(uuid, cmpInst);
    }
  }

  // ERC.
  {
    const QString fp = "circuit/erc.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeErc(root, context);
    dir.write(fp, root.toByteArray());
  }

  // Schematics.
  foreach (const QString& dirName, dir.getDirs("schematics")) {
    const QString fp = "schematics/" % dirName % "/schematic.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
      upgradeSchematic(root, context);
      dir.write(fp, root.toByteArray());
    }
  }

  // Boards.
  foreach (const QString& dirName, dir.getDirs("boards")) {
    // Board content.
    QString fp = "boards/" % dirName % "/board.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
      upgradeBoard(root, context);
      dir.write(fp, root.toByteArray());
    }

    // User settings.
    fp = "boards/" % dirName % "/settings.user.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
      upgradeBoardUserSettings(root);
      dir.write(fp, root.toByteArray());
    }
  }

  // Emit messages at the very end to avoid duplicate messages caused my
  // multiple schematics/boards.
  if (context.componentsWithAssemblyOptions > 0) {
    messages.append(buildMessage(
        Message::Severity::Note,
        tr("Components were automatically populated with assembly information "
           "required for the new, built-in MPN management and assembly variant "
           "mechanism. If the BOM or PnP export is used, please review the "
           "output and correct MPNs and attributes manually in the component "
           "properties dialog where needed."),
        context.componentsWithAssemblyOptions));
  }
  if (context.removedErcApprovals > 0) {
    messages.append(buildMessage(
        Message::Severity::Note,
        tr("Some particular ERC message approvals cannot be migrated and "
           "therefore have been removed. Please check the remaining ERC "
           "messages and approve them if desired."),
        context.removedErcApprovals));
  }
  if (context.holesCount > 0) {
    messages.append(
        buildMessage(Message::Severity::Note,
                     tr("All non-plated holes (NPTH) now have automatic stop "
                        "mask added on both board sides. The expansion value "
                        "is determined by the boards design rule settings but "
                        "can be overridden in the hole properties dialog."),
                     context.holesCount));
  }
  if (context.nonRoundViaCount > 0) {
    messages.append(
        buildMessage(Message::Severity::Warning,
                     tr("Non-circular via shapes are no longer supported, "
                        "all vias were changed to circular now."),
                     context.nonRoundViaCount));
  }
  if (context.planeCount > 0) {
    messages.append(buildMessage(
        Message::Severity::Note,
        tr("Plane area calculations have been adjusted, manual review and "
           "running the DRC is recommended."),
        context.planeCount));
  }
  if (context.planeConnectNoneCount > 0) {
    messages.append(buildMessage(
        Message::Severity::Warning,
        tr("Vias within planes with connect style 'None' are now fully "
           "connected to the planes since the connect style is no longer "
           "respected for vias. You might want to remove traces now which are "
           "no longer needed to connect these vias."),
        context.planeConnectNoneCount));
  }
  if ((context.footprintBoardOutlinesObjectCount > 0) ||
      (context.topLevelBoardOutlinesObjectCount > 1)) {
    messages.append(buildMessage(
        Message::Severity::Warning,
        tr("Board cutouts now have a dedicated layer, thus nested board "
           "outline polygons and circles have automatically been moved to the "
           "cutouts layer. As the auto-detection is not perfect, please "
           "check if each cutout has been converted correctly. The easiest way "
           "is to review the PCB in the 3D viewer."),
        context.footprintBoardOutlinesObjectCount +
            context.topLevelBoardOutlinesObjectCount));
  }
}

void FileFormatMigrationV01::upgradeWorkspaceData(TransactionalDirectory& dir) {
  // Create version file.
  dir.write(".librepcb-data", VersionFile(mToVersion).toByteArray());

  // Remove legacy files.
  const QStringList filesToRemove = {
      "cache",
      "cache_v1",
      "cache_v2",
      "library_cache",
  };
  TransactionalDirectory librariesDir(dir, "libraries");
  foreach (const QString fileName, librariesDir.getFiles()) {
    if (filesToRemove.contains(fileName.split(".").first())) {
      qInfo() << "Removing legacy file:"
              << librariesDir.getAbsPath(fileName).toNative();
      librariesDir.removeFile(fileName);
    }
  }

  // Upgrade settings.
  const QString settingsFp = "settings.lp";
  if (dir.fileExists(settingsFp)) {
    SExpression root =
        SExpression::parse(dir.read(settingsFp), dir.getAbsPath(settingsFp));
    if (SExpression* node = root.tryGetChild("repositories")) {
      foreach (SExpression* child, node->getChildren("repository")) {
        child->setName("url");
      }
      node->setName("api_endpoints");
    }
    root.replaceRecursive(SExpression::createToken("board_placement_top"),
                          SExpression::createToken("board_legend_top"));
    root.replaceRecursive(SExpression::createToken("board_placement_bottom"),
                          SExpression::createToken("board_legend_bottom"));
    dir.write(settingsFp, root.toByteArray());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileFormatMigrationV01::upgradeSettings(SExpression& root) {
  upgradeStrings(root);
  root.appendList("custom_bom_attributes");
  root.appendChild("default_lock_component_assembly",
                   SExpression::createToken("false"));
}

void FileFormatMigrationV01::upgradeCircuit(SExpression& root,
                                            ProjectContext& context) {
  upgradeStrings(root);

  // Add default assembly variant. Use the project's UUID as assembly variant
  // UUID to make the migration deterministic.
  {
    SExpression& node = root.appendList("variant");
    node.appendChild(SExpression::createToken(context.projectUuid));
    node.appendChild("name", SExpression::createString("Std"));
    node.appendChild("description",
                     SExpression::createString("Standard assembly"));
  }

  // Add assembly options & parts to components.
  foreach (SExpression* cmpNode, root.getChildren("component")) {
    const Uuid cmpUuid = deserialize<Uuid>(cmpNode->getChild("@0"));
    const Uuid libCmpUuid =
        deserialize<Uuid>(cmpNode->getChild("lib_component/@0"));
    const bool isLogo =
        (libCmpUuid.toStr() == "b91cf23a-4f07-4b99-8f52-0b42304aef20");
    const bool addToAssemblyVariant =
        (!context.components.value(libCmpUuid).schematicOnly) && (!isLogo);
    const SExpression& libDevNode = cmpNode->getChild("lib_device");
    QSet<Uuid> libDeviceUuids = context.devicesUsedInBoards.value(cmpUuid);
    if (auto u = deserialize<tl::optional<Uuid>>(libDevNode.getChild("@0"))) {
      libDeviceUuids.insert(*u);
    }

    if (!libDeviceUuids.isEmpty()) {
      QString mpn, manufacturer;
      QVector<SExpression*> consumedAttributes;
      const QList<SExpression*> attributes = cmpNode->getChildren("attribute");
      for (int i = attributes.count() - 1; i >= 0; --i) {
        const QString key = attributes.at(i)->getChild("@0").getValue();
        if (key == "MPN") {
          mpn = *cleanSimpleString(
              attributes.at(i)->getChild("value/@0").getValue());
          consumedAttributes.append(attributes.at(i));
        } else if (key == "MANUFACTURER") {
          manufacturer = *cleanSimpleString(
              attributes.at(i)->getChild("value/@0").getValue());
          consumedAttributes.append(attributes.at(i));
        }
      }
      foreach (SExpression* attrNode, consumedAttributes) {
        cmpNode->removeChild(*attrNode);
      }

      foreach (const Uuid& devUuid, libDeviceUuids) {
        SExpression& devNode = cmpNode->appendList("device");
        devNode.appendChild(SExpression::createToken(devUuid.toStr()));
        if ((!mpn.isEmpty()) || (!manufacturer.isEmpty())) {
          SExpression& partNode = devNode.appendList("part");
          partNode.appendChild(mpn);
          partNode.appendChild("manufacturer", manufacturer);
        }
        if (addToAssemblyVariant) {
          devNode.appendChild("variant",
                              SExpression::createToken(context.projectUuid));
        }
      }
      ++context.componentsWithAssemblyOptions;
    }

    cmpNode->removeChild(libDevNode);
    cmpNode->appendChild("lock_assembly", SExpression::createToken("false"));
  }
}

void FileFormatMigrationV01::upgradeErc(SExpression& root,
                                        ProjectContext& context) {
  SExpression newRoot = SExpression::createList(root.getName());
  for (const SExpression* node : root.getChildren("approved")) {
    const QString msgClass = node->getChild("class/@0").getValue();
    const QString instance = node->getChild("instance/@0").getValue();
    const QString message = node->getChild("message/@0").getValue();
    if ((msgClass == "NetClass") && (message == "Unused")) {
      SExpression& child = newRoot.appendList("approved");
      child.appendChild(SExpression::createToken("unused_netclass"));
      child.appendChild("netclass", SExpression::createToken(instance));
    } else if ((msgClass == "NetSignal") && (message == "Unused")) {
      SExpression& child = newRoot.appendList("approved");
      child.appendChild(SExpression::createToken("open_net"));
      child.appendChild("net", SExpression::createToken(instance));
    } else if ((msgClass == "NetSignal") &&
               (message == "ConnectedToLessThanTwoPins")) {
      SExpression& child = newRoot.appendList("approved");
      child.appendChild(SExpression::createToken("open_net"));
      child.appendChild("net", SExpression::createToken(instance));
    } else if (message == "UnconnectedRequiredSignal") {
      SExpression& child = newRoot.appendList("approved");
      child.appendChild(
          SExpression::createToken("unconnected_required_signal"));
      child.ensureLineBreak();
      child.appendChild("component",
                        SExpression::createToken(instance.split("/").first()));
      child.ensureLineBreak();
      child.appendChild("signal",
                        SExpression::createToken(instance.split("/").last()));
      child.ensureLineBreak();
    } else if (message == "ForcedNetSignalNameConflict") {
      SExpression& child = newRoot.appendList("approved");
      child.appendChild(
          SExpression::createToken("unconnected_required_signal"));
      child.ensureLineBreak();
      child.appendChild("component",
                        SExpression::createToken(instance.split("/").first()));
      child.ensureLineBreak();
      child.appendChild("signal",
                        SExpression::createToken(instance.split("/").last()));
      child.ensureLineBreak();
    } else {
      ++context.removedErcApprovals;
    }
  }
  root = newRoot;
}

void FileFormatMigrationV01::upgradeSchematic(SExpression& root,
                                              ProjectContext& context) {
  upgradeStrings(root);
  upgradeGrid(root);
  upgradeLayers(root);

  // Symbols.
  for (SExpression* symNode : root.getChildren("symbol")) {
    const Uuid cmpUuid = deserialize<Uuid>(symNode->getChild("component/@0"));
    const Uuid gateUuid = deserialize<Uuid>(symNode->getChild("lib_gate/@0"));
    auto cmpInstIt = context.componentInstances.find(cmpUuid);
    if (cmpInstIt == context.componentInstances.end()) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Failed to find component instance '%1'.")
                             .arg(cmpUuid.toStr()));
    }
    auto libCmpIt = context.components.find(cmpInstIt->libCmpUuid);
    if (libCmpIt == context.components.end()) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Failed to find component '%1'.")
                             .arg(cmpInstIt->libCmpUuid.toStr()));
    }
    const ComponentSymbolVariant* cmpSymbVar = nullptr;
    foreach (const auto& var, libCmpIt->symbolVariants) {
      if (var.uuid == cmpInstIt->libSymbVarUuid) {
        cmpSymbVar = &var;
        break;
      }
    }
    if (!cmpSymbVar) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Failed to find component symbol variant '%1'.")
              .arg(cmpInstIt->libSymbVarUuid.toStr()));
    }
    const Gate* gate = nullptr;
    foreach (const auto& g, cmpSymbVar->gates) {
      if (g.uuid == gateUuid) {
        gate = &g;
        break;
      }
    }
    if (!gate) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Failed to find gate '%1'.").arg(gateUuid.toStr()));
    }
    auto symIt = context.symbols.find(gate->symbolUuid);
    if (symIt == context.symbols.end()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Failed to find symbol '%1'.").arg(gate->symbolUuid.toStr()));
    }
    const Point symPos(symNode->getChild("position"));
    const Angle symRot = deserialize<Angle>(symNode->getChild("rotation/@0"));
    const bool symMirror = deserialize<bool>(symNode->getChild("mirror/@0"));
    foreach (const Text& text, symIt->texts) {
      Point position = text.position.rotated(symRot);
      if (symMirror) {
        position.mirror(Qt::Horizontal);
      }
      position += symPos;
      Angle rotation = symMirror ? (Angle::deg180() - symRot - text.rotation)
                                 : (symRot + text.rotation);
      Alignment align = text.align;
      if (symMirror) {
        align.mirrorV();
      }

      SExpression& textNode = symNode->appendList("text");
      textNode.appendChild(text.uuid);
      textNode.appendChild("layer", text.layerName);
      textNode.appendChild("value", text.text);
      align.serialize(textNode.appendList("align"));
      textNode.appendChild("height", text.height);
      position.serialize(textNode.appendList("position"));
      textNode.appendChild("rotation", rotation);
    }

    // Swap transformation order of mirror/rotate.
    if (symMirror) {
      SExpression& rotNode = symNode->getChild("rotation/@0");
      rotNode = serialize(-deserialize<Angle>(rotNode));
    }
  }

  // Net segments.
  for (SExpression* segNode : root.getChildren("netsegment")) {
    // Net labels.
    for (SExpression* lblNode : segNode->getChildren("label")) {
      lblNode->appendChild("mirror", false);
    }
  }
}

void FileFormatMigrationV01::upgradeBoard(SExpression& root,
                                          ProjectContext& context) {
  upgradeStrings(root);
  upgradeGrid(root);
  upgradeBoardDesignRules(root);
  upgradeBoardDrcSettings(root);
  upgradeLayers(root);
  upgradeCutouts(root, &context);

  // Board setup.
  root.appendChild("thickness", SExpression::createToken("1.6"));
  root.appendChild("solder_resist", SExpression::createToken("green"));
  root.appendChild("silkscreen", SExpression::createToken("white"));

  // Fabrication output settings.
  {
    SExpression& node = root.getChild("fabrication_output_settings");
    SExpression& drillNode = node.getChild("drills");
    drillNode.appendChild("g85_slots", false);
    if (drillNode.getChild("suffix_merged/@0").getValue().contains("DRILLS")) {
      // Default suffixes.
      drillNode.appendChild(
          "suffix_buried",
          QString("_DRILLS-PLATED-{{START_LAYER}}-{{END_LAYER}}.drl"));
    } else {
      // Protel suffixes.
      drillNode.appendChild("suffix_buried",
                            QString("_L{{START_NUMBER}}-L{{END_NUMBER}}.drl"));
    }

    SExpression& silkTop = node.getChild("silkscreen_top");
    SExpression& silkLayersTop = silkTop.getChild("layers");
    root.appendChild(SExpression(silkLayersTop))
        .setName("silkscreen_layers_top");
    silkTop.removeChild(silkLayersTop);

    SExpression& silkBot = node.getChild("silkscreen_bot");
    SExpression& silkLayersBot = silkBot.getChild("layers");
    root.appendChild(SExpression(silkLayersBot))
        .setName("silkscreen_layers_bot");
    silkBot.removeChild(silkLayersBot);
  }

  // Devices.
  for (SExpression* devNode : root.getChildren("device")) {
    if (deserialize<bool>(devNode->getChild("mirror/@0"))) {
      SExpression& rotNode = devNode->getChild("rotation/@0");
      rotNode = serialize(-deserialize<Angle>(rotNode));
    }
    devNode->appendChild("lock", SExpression::createToken("false"));
    for (SExpression* txtNode : devNode->getChildren("stroke_text")) {
      if (deserialize<bool>(txtNode->getChild("mirror/@0"))) {
        SExpression& rotNode = txtNode->getChild("rotation/@0");
        rotNode = serialize(-deserialize<Angle>(rotNode));
      }
      txtNode->appendChild("lock", SExpression::createToken("false"));
    }
    devNode->getChild("mirror").setName("flip");
    devNode->appendChild("lib_3d_model", SExpression::createToken("none"));
  }

  // Net segments.
  const UnsignedLength stopMaskMaxViaDiameter = deserialize<UnsignedLength>(
      root.getChild("design_rules/stopmask_max_via_drill_diameter/@0"));
  for (SExpression* segNode : root.getChildren("netsegment")) {
    // Vias.
    for (SExpression* viaNode : segNode->getChildren("via")) {
      SExpression& shapeNode = viaNode->getChild("shape");
      if (shapeNode.getChild("@0").getValue() != "round") {
        ++context.nonRoundViaCount;
      }
      viaNode->removeChild(shapeNode);
      viaNode->appendChild("from", SExpression::createToken("top_cu"));
      viaNode->appendChild("to", SExpression::createToken("bot_cu"));
      const PositiveLength drill =
          deserialize<PositiveLength>(viaNode->getChild("drill/@0"));
      if (drill > stopMaskMaxViaDiameter) {
        viaNode->appendChild("exposure", SExpression::createToken("auto"));
      } else {
        viaNode->appendChild("exposure", SExpression::createToken("off"));
      }
    }
  }

  // Polygons.
  for (SExpression* polyNode : root.getChildren("polygon")) {
    polyNode->appendChild("lock", SExpression::createToken("false"));
  }

  // Stroke texts.
  for (SExpression* txtNode : root.getChildren("stroke_text")) {
    if (deserialize<bool>(txtNode->getChild("mirror/@0"))) {
      SExpression& rotNode = txtNode->getChild("rotation/@0");
      rotNode = serialize(-deserialize<Angle>(rotNode));
    }
    txtNode->appendChild("lock", SExpression::createToken("false"));
  }

  // Holes.
  context.holesCount += root.getChildren("hole").count();
  upgradeHoles(root, true);

  // Planes.
  for (SExpression* planeNode : root.getChildren("plane")) {
    Q_UNUSED(planeNode);
    ++context.planeCount;
    if (planeNode->getChild("connect_style/@0").getValue() == "none") {
      ++context.planeConnectNoneCount;
    }
    planeNode->appendChild("thermal_gap",
                           planeNode->getChild("min_clearance/@0"));
    planeNode->appendChild("thermal_spoke",
                           planeNode->getChild("min_width/@0"));
    planeNode->appendChild("lock", SExpression::createToken("false"));
    planeNode->getChild("keep_orphans").setName("keep_islands");
  }
}

void FileFormatMigrationV01::upgradeBoardUserSettings(SExpression& root) {
  upgradeLayers(root);

  // Layer colors.
  foreach (SExpression* node, root.getChildren("layer")) {
    for (const auto tagName : {"color", "color_hl"}) {
      if (SExpression* child = node->tryGetChild(tagName)) {
        node->removeChild(*child);
      }
    }
  }
}

void FileFormatMigrationV01::upgradeBoardDesignRules(SExpression& root) {
  SExpression& node = root.getChild("design_rules");
  node.removeChild(node.getChild("name"));
  node.removeChild(node.getChild("description"));
  for (SExpression* child : node.getChildren(SExpression::Type::List)) {
    QString name = child->getName();
    name.replace("restring_pad_", "pad_annular_ring_");
    name.replace("restring_via_", "via_annular_ring_");
    name.replace("creammask_", "solderpaste_");
    child->setName(name);
  }
  for (const QString param : {"stopmask_clearance", "solderpaste_clearance",
                              "pad_annular_ring", "via_annular_ring"}) {
    SExpression& newChild = node.appendList(param);
    for (const QString property : {"ratio", "min", "max"}) {
      SExpression& oldChild = node.getChild(param % "_" % property);
      newChild.appendChild(property, oldChild.getChild("@0"));
      node.removeChild(oldChild);
    }
  }
  {
    SExpression& child = node.getChild("pad_annular_ring");
    child.appendChild("outer", SExpression::createToken("full"));
    child.appendChild("inner", SExpression::createToken("full"));
  }
}

void FileFormatMigrationV01::upgradeBoardDrcSettings(SExpression& root) {
  SExpression& node = root.appendList("design_rule_check");
  node.appendChild("min_copper_copper_clearance",
                   SExpression::createToken("0.2"));
  node.appendChild("min_copper_board_clearance",
                   SExpression::createToken("0.3"));
  node.appendChild("min_copper_npth_clearance",
                   SExpression::createToken("0.25"));
  node.appendChild("min_drill_drill_clearance",
                   SExpression::createToken("0.35"));
  node.appendChild("min_drill_board_clearance",
                   SExpression::createToken("0.5"));
  node.appendChild("min_silkscreen_stopmask_clearance",
                   SExpression::createToken("0.127"));
  node.appendChild("min_copper_width", SExpression::createToken("0.2"));
  node.appendChild("min_annular_ring", SExpression::createToken("0.2"));
  node.appendChild("min_npth_drill_diameter", SExpression::createToken("0.3"));
  node.appendChild("min_pth_drill_diameter", SExpression::createToken("0.3"));
  node.appendChild("min_npth_slot_width", SExpression::createToken("1.0"));
  node.appendChild("min_pth_slot_width", SExpression::createToken("0.7"));
  node.appendChild("min_silkscreen_width", SExpression::createToken("0.15"));
  node.appendChild("min_silkscreen_text_height",
                   SExpression::createToken("0.8"));
  node.appendChild("min_outline_tool_diameter",
                   SExpression::createToken("2.0"));
  node.appendChild("blind_vias_allowed", SExpression::createToken("false"));
  node.appendChild("buried_vias_allowed", SExpression::createToken("false"));
  node.appendChild("allowed_npth_slots",
                   SExpression::createToken("single_segment_straight"));
  node.appendChild("allowed_pth_slots",
                   SExpression::createToken("single_segment_straight"));
  node.appendChild("approvals_version", SExpression::createToken("0.2"));
}

void FileFormatMigrationV01::upgradeGrid(SExpression& node) {
  SExpression& gridNode = node.getChild("grid");
  gridNode.removeChild(gridNode.getChild("type"));
}

void FileFormatMigrationV01::upgradeCutouts(SExpression& node,
                                            ProjectContext* context) {
  // Collect all outline objects.
  struct OutlineObject {
    SExpression* node;
    Path outline;
    qreal lengthMm;
  };
  QVector<OutlineObject> outlineObjects;
  foreach (SExpression* child, node.getChildren("polygon")) {
    if (child->getChild("layer/@0").getValue() == "brd_outlines") {
      OutlineObject obj{child, Path(*child), 0};
      obj.lengthMm = obj.outline.getTotalStraightLength()->toMm();
      outlineObjects.append(obj);
    }
  }
  foreach (SExpression* child, node.getChildren("circle")) {
    if (child->getChild("layer/@0").getValue() == "brd_outlines") {
      const Point position(*child);
      const PositiveLength diameter =
          deserialize<PositiveLength>(child->getChild("diameter/@0"));
      OutlineObject obj{child, Path::circle(diameter).translated(position),
                        diameter->toMm() * M_PI};
      outlineObjects.append(obj);
    }
  }

  // Sort by outline length ascending.
  std::sort(outlineObjects.begin(), outlineObjects.end(),
            [](const OutlineObject& a, const OutlineObject& b) {
              return a.lengthMm < b.lengthMm;
            });

  // Discard the outline which is considered as the outer most board outline.
  if (!outlineObjects.isEmpty()) {
    if (context) {
      // In boards, the longest outline is considered as the board outlines.
      outlineObjects.removeLast();
      context->topLevelBoardOutlinesObjectCount += outlineObjects.count();
    } else {
      // In footprints, the longest outline is only considered as the board
      // outlines if there is any pad located *within* the outline.
      foreach (const SExpression* padNode, node.getChildren("pad")) {
        const Point padPosition(padNode->getChild("position"));
        if (outlineObjects.last().outline.toQPainterPathPx().contains(
                padPosition.toPxQPointF())) {
          outlineObjects.removeLast();
          break;
        }
      }
    }
  }

  // Move all remaining outlines to the new cutouts layer.
  foreach (const OutlineObject& obj, outlineObjects) {
    obj.node->getChild("layer/@0").setValue("brd_cutouts");
  }
}

void FileFormatMigrationV01::upgradeHoles(SExpression& node, bool isBoardHole) {
  for (SExpression* holeNode : node.getChildren("hole")) {
    holeNode->appendChild("stop_mask", SExpression::createToken("auto"));
    const Point pos(holeNode->getChild("position"));
    SExpression& vertexNode = holeNode->appendList("vertex");
    pos.serialize(vertexNode.appendList("position"));
    vertexNode.appendChild("angle", Angle::deg0());
    if (isBoardHole) {
      holeNode->appendChild("lock", SExpression::createToken("false"));
    }
  }
}

void FileFormatMigrationV01::upgradeLayers(SExpression& node) {
  // Rename "sch_scheet_frames" to "sch_frames".
  node.replaceRecursive(SExpression::createToken("sch_scheet_frames"),
                        SExpression::createToken("sch_frames"));

  // Rename "brd_sheet_frames" to "brd_frames".
  node.replaceRecursive(SExpression::createToken("brd_sheet_frames"),
                        SExpression::createToken("brd_frames"));

  // Rename "brd_milling_pth" to "brd_plated_cutouts".
  node.replaceRecursive(SExpression::createToken("brd_milling_pth"),
                        SExpression::createToken("brd_plated_cutouts"));

  // Remove nodes on never officially existing layer "brd_keepout".
  SExpression search = SExpression::createList("layer");
  search.appendChild(SExpression::createToken("brd_keepout"));
  node.removeChildrenWithNodeRecursive(search);

  // Rename "top_placement" to "top_legend".
  node.replaceRecursive(SExpression::createToken("top_placement"),
                        SExpression::createToken("top_legend"));

  // Rename "bot_placement" to "bot_legend".
  node.replaceRecursive(SExpression::createToken("bot_placement"),
                        SExpression::createToken("bot_legend"));
}

void FileFormatMigrationV01::upgradeInversionCharacters(
    SExpression& root, const QString& childName, const QString& valuePath) {
  QSet<QString> reservedValues;
  foreach (SExpression* child, root.getChildren(childName)) {
    reservedValues.insert(child->getChild(valuePath).getValue());
  }
  foreach (SExpression* child, root.getChildren(childName)) {
    SExpression& node = child->getChild(valuePath);
    const QString newValue = "!" % node.getValue().mid(1);
    if (node.getValue().startsWith("/") &&
        (!reservedValues.contains(newValue))) {
      node.setValue(newValue);
    }
  }
}

void FileFormatMigrationV01::upgradeStrings(SExpression& root) {
  QMap<QString, QString> replacements = {
      {"MODIFIED_DATE", "DATE"},
      {"MODIFIED_TIME", "TIME"},
      {"PARTNUMBER", "MPN"},
  };
  replaceStrings(root, replacements);
}

void FileFormatMigrationV01::replaceStrings(
    SExpression& root, const QMap<QString, QString>& replacements) {
  foreach (SExpression* child, root.getChildren(SExpression::Type::List)) {
    replaceStrings(*child, replacements);
  }
  foreach (SExpression* child, root.getChildren(SExpression::Type::String)) {
    QString s = child->getValue();
    for (auto it = replacements.begin(); it != replacements.end(); it++) {
      s.replace(it.key(), it.value());
    }
    child->setValue(s);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
