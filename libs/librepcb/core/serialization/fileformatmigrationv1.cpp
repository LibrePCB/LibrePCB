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
#include "fileformatmigrationv1.h"

#include "../fileio/transactionaldirectory.h"
#include "../fileio/versionfile.h"
#include "sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileFormatMigrationV1::FileFormatMigrationV1(QObject* parent) noexcept
  : FileFormatMigration(Version::fromString("1"), Version::fromString("2"),
                        parent) {
}

FileFormatMigrationV1::~FileFormatMigrationV1() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FileFormatMigrationV1::upgradeComponentCategory(
    TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-cmpcat");
}

void FileFormatMigrationV1::upgradePackageCategory(
    TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-pkgcat");
}

void FileFormatMigrationV1::upgradeSymbol(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-sym");

  // Content File.
  {
    const QString fp = "symbol.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root->appendChild("grid_interval", SExpression::createToken("2.54"));
    dir.write(fp, root->toByteArray());
  }
}

void FileFormatMigrationV1::upgradePackage(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-pkg");

  // Content File.
  {
    const QString fp = "package.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    root->appendChild("grid_interval", SExpression::createToken("2.54"));

    // Footprints.
    for (SExpression* fptNode : root->getChildren("footprint")) {
      // Pads.
      for (SExpression* padNode : fptNode->getChildren("pad")) {
        // Revert possibly made manual change as a workaround for bug, see
        // https://librepcb.discourse.group/t/migrating-libraries-from-old-version-1-0-pressfit-problem/810
        SExpression& padFunction = padNode->getChild("function/@0");
        if (padFunction.getValue() == "press_fit") {
          padFunction.setValue("pressfit");
        }
      }
    }

    dir.write(fp, root->toByteArray());
  }
}

void FileFormatMigrationV1::upgradeComponent(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-cmp");
}

void FileFormatMigrationV1::upgradeDevice(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-dev");
}

void FileFormatMigrationV1::upgradeLibrary(TransactionalDirectory& dir) {
  // Version File.
  upgradeVersionFile(dir, ".librepcb-lib");
}

void FileFormatMigrationV1::upgradeProject(TransactionalDirectory& dir,
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

  // Metadata.
  {
    const QString fp = "project/metadata.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeMetadata(*root, messages);
    dir.write(fp, root->toByteArray());
  }

  // Settings.
  {
    const QString fp = "project/settings.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeSettings(*root, messages);
    dir.write(fp, root->toByteArray());
  }

  // Output Jobs.
  {
    const QString fp = "project/jobs.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeOutputJobs(*root, context);
    dir.write(fp, root->toByteArray());
  }

  // Circuit.
  {
    const QString fp = "circuit/circuit.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    upgradeCircuit(*root, messages);
    dir.write(fp, root->toByteArray());
  }

  // Boards.
  foreach (const QString& dirName, dir.getDirs("boards")) {
    QString fp = "boards/" % dirName % "/board.lp";
    if (dir.fileExists(fp)) {
      ++context.boardCount;
      std::unique_ptr<SExpression> root =
          SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
      upgradeBoard(*root);
      dir.write(fp, root->toByteArray());
    }
  }

  // Emit messages at the very end to avoid duplicate messages caused my
  // multiple schematics/boards.
  if ((context.boardCount > 0) && (!context.hasGerberOutputJob)) {
    messages.append(buildMessage(
        Message::Severity::Warning,
        tr("The dedicated Gerber/Excellon generator dialog has been removed "
           "in favor of the more powerful output jobs, and the corresponding "
           "output settings will be removed from boards in an upcoming "
           "release. It is recommended to add a Gerber/Excellon output job "
           "now, as this allows to migrate the old export settings."),
        1));
  }
}

void FileFormatMigrationV1::upgradeWorkspaceData(TransactionalDirectory& dir) {
  // Create version file.
  dir.write(".librepcb-data", VersionFile(mToVersion).toByteArray());

  // Remove legacy files.
  const QStringList filesToRemove = {
      "cache_v3",
      "cache_v4",
      "cache_v5",
      "cache_v6",
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
 *  Protected Methods
 ******************************************************************************/

void FileFormatMigrationV1::upgradeMetadata(SExpression& root,
                                            QList<Message>& messages) {
  // FileProofName does no longer allow string consisting of only dots
  // (e.g. "..") so we rename them.
  SExpression& versionNode = root.getChild("version/@0");
  if (auto newVersion = upgradeFileProofName(versionNode.getValue())) {
    versionNode.setValue(*newVersion);
    // Not translated because it's unlikely someone will ever see this message.
    messages.append(buildMessage(
        Message::Severity::Note,
        "Project version has been adjusted due to more restrictive naming "
        "requirements. Please review the new version number.",
        1));
  }
}

void FileFormatMigrationV1::upgradeSettings(SExpression& root,
                                            QList<Message>& messages) {
  // The manual BOM export has been removed. If the user has configured custom
  // BOM attributes, just remind him to use output jobs now.
  QStringList customBomAttributes;
  for (const SExpression* node :
       root.getChild("custom_bom_attributes").getChildren("attribute")) {
    customBomAttributes.append(node->getChild("@0").getValue());
  }
  if (!customBomAttributes.isEmpty()) {
    messages.append(buildMessage(
        Message::Severity::Note,
        tr("The project has set custom attributes for the BOM export (%1). But "
           "in LibrePCB 2.0, the manual BOM export has been removed in favor "
           "of the more powerful output jobs feature. Please use output jobs "
           "now to generate the BOM. When you add a new BOM output job, those "
           "custom attributes will automatically be imported.")
            .arg(customBomAttributes.join(", ")),
        1));
  }
}

void FileFormatMigrationV1::upgradeOutputJobs(SExpression& root,
                                              ProjectContext& context) {
  for (SExpression* jobNode : root.getChildren("job")) {
    if (jobNode->getChild("type/@0").getValue() == "graphics") {
      for (SExpression* contentNode : jobNode->getChildren("content")) {
        SExpression& contentTypeNode = contentNode->getChild("type/@0");
        if (contentTypeNode.getValue() == "schematic") {
          // Add the new layer for image borders.
          SExpression& imgBordersLayerNode = contentNode->appendList("layer");
          imgBordersLayerNode.appendChild(
              SExpression::createToken("schematic_image_borders"));
          imgBordersLayerNode.appendChild(
              "color", SExpression::createString("#ff808080"));
        } else if (contentTypeNode.getValue() == "board") {
          // We don't need to check the option value since "realistic" was
          // the only supported option in v1.
          const auto optionNodes = contentNode->getChildren("option");
          for (SExpression* optionNode : optionNodes) {
            contentNode->removeChild(*optionNode);
          }
          if (!optionNodes.isEmpty()) {
            contentTypeNode.setValue("board_rendering");
            for (SExpression* layerNode : contentNode->getChildren("layer")) {
              contentNode->removeChild(*layerNode);
            }
            auto addLayer = [&contentNode](const QString& layer,
                                           const QString& color) {
              SExpression& node = contentNode->appendList("layer");
              node.appendChild(SExpression::createToken(layer));
              node.appendChild("color", color);
            };
            if (contentNode->getChild("mirror/@0").getValue() == "true") {
              addLayer("board_copper_bottom", "#ffbc9c69");
              addLayer("board_legend_bottom", "#00000000");
              addLayer("board_outlines", "#ff465046");
              addLayer("board_stop_mask_bottom", "#00000000");
            } else {
              addLayer("board_copper_top", "#ffbc9c69");
              addLayer("board_legend_top", "#00000000");
              addLayer("board_outlines", "#ff465046");
              addLayer("board_stop_mask_top", "#00000000");
            }
          }
        }
      }
    } else if (jobNode->getChild("type/@0").getValue() == "gerber_excellon") {
      context.hasGerberOutputJob = true;
    }
  }
}

void FileFormatMigrationV1::upgradeCircuit(SExpression& root,
                                           QList<Message>& messages) {
  // Assembly variants.
  int renamedAssemblyVariants = 0;
  for (SExpression* variantNode : root.getChildren("variant")) {
    // FileProofName does no longer allow string consisting of only dots
    // (e.g. "..") so we rename them. We don't do conflict resolution here
    // as it is very unlikely to ever happen.
    SExpression& nameNode = variantNode->getChild("name/@0");
    if (auto newName = upgradeFileProofName(nameNode.getValue())) {
      nameNode.setValue(*newName);
      ++renamedAssemblyVariants;
    }
  }
  if (renamedAssemblyVariants > 0) {
    // Not translated because it's unlikely someone will ever see this message.
    messages.append(buildMessage(
        Message::Severity::Note,
        "Assembly variants have been renamed due to more restrictive naming "
        "requirements. Please review the new names.",
        renamedAssemblyVariants));
  }
}

void FileFormatMigrationV1::upgradeBoard(SExpression& root) {
  // DRC approvals.
  SExpression& drcNode = root.getChild("design_rule_check");
  const QString approvalsVersion =
      drcNode.getChild("approvals_version/@0").getValue();
  for (SExpression* approvalNode : drcNode.getChildren("approved")) {
    SExpression& approvalTypeNode = approvalNode->getChild("@0");
    if ((approvalTypeNode.getValue() == "useless_via") &&
        (approvalsVersion != "2")) {
      approvalTypeNode.setValue("invalid_via");
    } else if (approvalTypeNode.getValue() == "antennae_via") {
      approvalTypeNode.setValue("useless_via");
    }
  }
}

std::optional<QString> FileFormatMigrationV1::upgradeFileProofName(
    QString name) {
  if (QRegularExpression("\\A\\.+\\z")
          .match(name, 0, QRegularExpression::PartialPreferCompleteMatch)
          .hasMatch()) {
    return name.replace(".", "_");
  }
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
