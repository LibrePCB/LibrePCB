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
  : FileFormatMigrationV01(parent) {
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
 *  Private Methods
 ******************************************************************************/

void FileFormatMigrationUnstable::upgradeSettings(SExpression& root) {
  Q_UNUSED(root);
}

void FileFormatMigrationUnstable::upgradeCircuit(SExpression& root,
                                                 ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
}

void FileFormatMigrationUnstable::upgradeErc(SExpression& root,
                                             ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
}

void FileFormatMigrationUnstable::upgradeSchematic(SExpression& root,
                                                   ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
}

void FileFormatMigrationUnstable::upgradeBoard(SExpression& root,
                                               ProjectContext& context) {
  Q_UNUSED(root);
  Q_UNUSED(context);
  upgradeBoardDrcSettings(root);
}

void FileFormatMigrationUnstable::upgradeBoardUserSettings(SExpression& root) {
  Q_UNUSED(root);
}

void FileFormatMigrationUnstable::upgradeBoardDrcSettings(SExpression& root) {
  SExpression& node = root.getChild("design_rule_check");
  node.appendChild("min_silkscreen_stopmask_clearance",
                   SExpression::createToken("0.127"));
  node.appendChild("min_silkscreen_width", SExpression::createToken("0.15"));
  node.appendChild("min_silkscreen_text_height",
                   SExpression::createToken("0.8"));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
