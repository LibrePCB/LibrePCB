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
#include "fileformatmigrationunstable.h"

#include "../application.h"
#include "../fileio/transactionaldirectory.h"
#include "../types/elementname.h"
#include "../types/simplestring.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileFormatMigrationUnstable::FileFormatMigrationUnstable(
    QObject* parent) noexcept
  : FileFormatMigrationV1(parent) {
  mFromVersion = mToVersion;  // Clearly distinguish from the base class.
}

FileFormatMigrationUnstable::~FileFormatMigrationUnstable() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FileFormatMigrationUnstable::upgradeComponentCategory(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradePackageCategory(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeSymbol(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradePackage(TransactionalDirectory& dir) {
  Q_UNUSED(dir);

  // Content File.
  {
    const QString fp = "package.lp";
    std::unique_ptr<SExpression> root =
        SExpression::parse(dir.read(fp), dir.getAbsPath(fp));
    for (SExpression* fptNode : root->getChildren("footprint")) {
      // Stroke texts.
      for (SExpression* child : fptNode->getChildren("stroke_text")) {
        const QSet<QString> unlockedLayers = {"top_names", "top_values",
                                              "bot_names", "bot_values"};
        const QString layer = child->getChild("layer/@0").getValue();
        const bool lock = !unlockedLayers.contains(layer);
        child->appendChild("lock",
                           SExpression::createToken(lock ? "true" : "false"));
      }
    }
    dir.write(fp, root->toByteArray());
  }
}

void FileFormatMigrationUnstable::upgradeComponent(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeDevice(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeLibrary(TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

void FileFormatMigrationUnstable::upgradeWorkspaceData(
    TransactionalDirectory& dir) {
  Q_UNUSED(dir);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void FileFormatMigrationUnstable::upgradeOutputJobs(SExpression& root,
                                                    ProjectContext& context) {
  Q_UNUSED(context);
  for (SExpression* jobNode : root.getChildren("job")) {
    if (jobNode->getChild("type/@0").getValue() == "graphics") {
      for (SExpression* contentNode : jobNode->getChildren("content")) {
        SExpression& contentTypeNode = contentNode->getChild("type/@0");
        if (contentTypeNode.getValue() == "schematic") {
          // Add the new layer for buses.
          SExpression& busesLayerNode = contentNode->appendList("layer");
          busesLayerNode.appendChild(
              SExpression::createToken("schematic_buses"));
          busesLayerNode.appendChild("color",
                                     SExpression::createString("#ff008eff"));
          // Add the new layer for bus labels.
          SExpression& busLabelsLayerNode = contentNode->appendList("layer");
          busLabelsLayerNode.appendChild(
              SExpression::createToken("schematic_bus_labels"));
          busLabelsLayerNode.appendChild(
              "color", SExpression::createString("#ff008eff"));
        }
      }
    }
  }
}

void FileFormatMigrationUnstable::upgradeCircuit(SExpression& root,
                                                 QList<Message>& messages) {
  Q_UNUSED(root);
  Q_UNUSED(messages);
}

void FileFormatMigrationUnstable::upgradeBoard(SExpression& root) {
  // DRC settings.
  SExpression& drcNode = root.getChild("design_rule_check");
  drcNode.ensureLineBreak();
  {
    SExpression& child = drcNode.appendList("min_pcb_size");
    child.appendChild(SExpression::createToken("0.0"));
    child.appendChild(SExpression::createToken("0.0"));
  }
  drcNode.ensureLineBreak();
  {
    SExpression& child = drcNode.appendList("max_pcb_size");
    SExpression& doubleSided = child.appendList("double_sided");
    doubleSided.appendChild(SExpression::createToken("0.0"));
    doubleSided.appendChild(SExpression::createToken("0.0"));
    SExpression& multilayer = child.appendList("multilayer");
    multilayer.appendChild(SExpression::createToken("0.0"));
    multilayer.appendChild(SExpression::createToken("0.0"));
  }
  drcNode.ensureLineBreak();
  drcNode.appendList("pcb_thickness");
  drcNode.ensureLineBreak();
  drcNode.appendChild("max_layers", SExpression::createToken("0"));
  drcNode.ensureLineBreak();
  drcNode.appendList("solder_resist");
  drcNode.ensureLineBreak();
  drcNode.appendList("silkscreen");
  drcNode.ensureLineBreak();
  drcNode.appendChild("max_tented_via_drill_diameter",
                      SExpression::createToken("0.5"));
  drcNode.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
