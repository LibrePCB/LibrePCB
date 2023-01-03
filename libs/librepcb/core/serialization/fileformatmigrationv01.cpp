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

void FileFormatMigrationV01::upgradeProject(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-project");

  // Symbols.
  foreach (const QString& dirName, dir.getDirs("library/sym")) {
    TransactionalDirectory subDir(dir, "library/sym/" % dirName);
    if (subDir.fileExists(".librepcb-sym")) {
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

  // Schematics.
  foreach (const QString& dirName, dir.getDirs("schematics")) {
    const QString fp = "schematics/" % dirName % "/schematic.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));

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
    const QString fp = "boards/" % dirName % "/board.lp";
    if (dir.fileExists(fp)) {
      SExpression root = SExpression::parse(dir.read(fp), dir.getAbsPath(fp));

      // Fabrication output settings.
      {
        SExpression& node = root.getChild("fabrication_output_settings");
        SExpression& drillNode = node.getChild("drills");
        drillNode.appendChild("g85_slots", false);
      }

      // Holes.
      upgradeHoles(root);

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
