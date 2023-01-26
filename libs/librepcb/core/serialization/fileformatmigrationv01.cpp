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
#include "../types/alignment.h"
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/point.h"
#include "../types/uuid.h"
#include "sexpression.h"

#include <QtCore>

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

    // Pins.
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

    // Footprints.
    for (SExpression* fptNode : root.getChildren("footprint")) {
      // Pads.
      for (SExpression* padNode : fptNode->getChildren("pad")) {
        // In the file format 0.1, footprint pads did not have their own UUID,
        // but only the UUID of the package pad they were connected to. To get
        // a deterministic UUID when upgrading a v0.1 footprint pad to v0.2,
        // we simply use the package pad UUID as the footprint pad UUID too.
        // See https://github.com/LibrePCB/LibrePCB/issues/445
        const Uuid uuid = deserialize<Uuid>(padNode->getChild("@0"));
        padNode->appendChild("package_pad", uuid);

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
      }

      // Holes.
      upgradeHoles(*fptNode);
    }

    dir.write(fp, root.toByteArray());
  }
}

void FileFormatMigrationV01::upgradeComponent(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-cmp");
}

void FileFormatMigrationV01::upgradeDevice(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-dev");
}

void FileFormatMigrationV01::upgradeLibrary(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-lib");
}

void FileFormatMigrationV01::upgradeProject(TransactionalDirectory& dir,
                                            QList<Message>& messages) {
  LoadedData data;

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
            deserialize<GraphicsLayerName>(textNode->getChild("layer/@0")),
            textNode->getChild("value/@0").getValue(),
            Point(textNode->getChild("position")),
            deserialize<Angle>(textNode->getChild("rotation/@0")),
            deserialize<PositiveLength>(textNode->getChild("height/@0")),
            Alignment(textNode->getChild("align")),
        });
      }

      data.symbols.insert(uuid, sym);

      upgradeSymbol(subDir);
    }
  }

  // Packages.
  foreach (const QString& dirName, dir.getDirs("library/pkg")) {
    TransactionalDirectory subDir(dir, "library/pkg/" % dirName);
    if (subDir.fileExists(".librepcb-pkg")) {
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

      data.components.insert(uuid, cmp);

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

  // Circuit.
  {
    const QString fp = "circuit/circuit.lp";
    SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));

    // Component instances.
    for (SExpression* cmpNode : root.getChildren("component")) {
      const Uuid uuid = deserialize<Uuid>(cmpNode->getChild("@0"));
      ComponentInstance cmpInst{
          deserialize<Uuid>(cmpNode->getChild("lib_component/@0")),
          deserialize<Uuid>(cmpNode->getChild("lib_variant/@0")),
      };
      data.componentInstances.insert(uuid, cmpInst);
    }
  }

  // Schematics.
  foreach (const QString& dirName, dir.getDirs("schematics")) {
    const QString fp = "schematics/" % dirName % "/schematic.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));

      upgradeGrid(root);

      // Symbols.
      for (SExpression* symNode : root.getChildren("symbol")) {
        const Uuid cmpUuid =
            deserialize<Uuid>(symNode->getChild("component/@0"));
        const Uuid gateUuid =
            deserialize<Uuid>(symNode->getChild("lib_gate/@0"));
        auto cmpInstIt = data.componentInstances.find(cmpUuid);
        if (cmpInstIt == data.componentInstances.end()) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Failed to find component instance '%1'.")
                                 .arg(cmpUuid.toStr()));
        }
        auto libCmpIt = data.components.find(cmpInstIt->libCmpUuid);
        if (libCmpIt == data.components.end()) {
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
        auto symIt = data.symbols.find(gate->symbolUuid);
        if (symIt == data.symbols.end()) {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Failed to find symbol '%1'.")
                                 .arg(gate->symbolUuid.toStr()));
        }
        const Point symPos(symNode->getChild("position"));
        const Angle symRot =
            deserialize<Angle>(symNode->getChild("rotation/@0"));
        const bool symMirror =
            deserialize<bool>(symNode->getChild("mirror/@0"));
        foreach (const Text& text, symIt->texts) {
          Point position = text.position.rotated(symRot);
          if (symMirror) {
            position.mirror(Qt::Horizontal);
          }
          position += symPos;
          Angle rotation = symMirror
              ? (Angle::deg180() - symRot - text.rotation)
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
      }

      // Net segments.
      for (SExpression* segNode : root.getChildren("netsegment")) {
        // Net labels.
        for (SExpression* lblNode : segNode->getChildren("label")) {
          lblNode->appendChild("mirror", false);
        }
      }

      dir.write(fp, root.toByteArray());
    }
  }

  // Boards.
  foreach (const QString& dirName, dir.getDirs("boards")) {
    // Board content.
    QString fp = "boards/" % dirName % "/board.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));

      upgradeGrid(root);

      // Fabrication output settings.
      {
        SExpression& node = root.getChild("fabrication_output_settings");
        SExpression& drillNode = node.getChild("drills");
        drillNode.appendChild("g85_slots", false);
      }

      // Holes.
      upgradeHoles(root);

      // Planes.
      int planeCount = 0;
      for (SExpression* planeNode : root.getChildren("plane")) {
        Q_UNUSED(planeNode);
        ++planeCount;
      }
      if (planeCount > 0) {
        messages.append(buildMessage(
            Message::Severity::Note,
            tr("Plane area calculations have been adjusted, manual review and "
               "running the DRC is recommended."),
            planeCount));
      }

      dir.write(fp, root.toByteArray());
    }

    // User settings.
    fp = "boards/" % dirName % "/settings.user.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));

      // Layers.
      foreach (SExpression* node, root.getChildren("layer")) {
        for (const auto tagName : {"color", "color_hl"}) {
          if (SExpression* child = node->tryGetChild(tagName)) {
            node->removeChild(*child);
          }
        }
      }

      dir.write(fp, root.toByteArray());
    }
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
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileFormatMigrationV01::upgradeGrid(SExpression& node) {
  SExpression& gridNode = node.getChild("grid");
  gridNode.removeChild(gridNode.getChild("type"));
}

void FileFormatMigrationV01::upgradeHoles(SExpression& node) {
  for (SExpression* holeNode : node.getChildren("hole")) {
    const Point pos(holeNode->getChild("position"));
    SExpression& vertexNode = holeNode->appendList("vertex");
    pos.serialize(vertexNode.appendList("position"));
    vertexNode.appendChild("angle", Angle::deg0());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
