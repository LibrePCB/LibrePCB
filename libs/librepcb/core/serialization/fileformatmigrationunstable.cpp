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
  Q_UNUSED(root);
  Q_UNUSED(context);
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
  drcNode.appendChild("max_inner_layers", SExpression::createToken("62"));
  drcNode.ensureLineBreak();
  drcNode.appendList("solder_resist");
  drcNode.ensureLineBreak();
  drcNode.appendList("silkscreen");
  drcNode.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
