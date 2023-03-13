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
#include "package.h"

#include "../../serialization/fileformatmigration.h"
#include "packagecheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const Package::AssemblyType& obj) {
  switch (obj) {
    case Package::AssemblyType::None:
      return SExpression::createToken("none");
    case Package::AssemblyType::Tht:
      return SExpression::createToken("tht");
    case Package::AssemblyType::Smt:
      return SExpression::createToken("smt");
    case Package::AssemblyType::Mixed:
      return SExpression::createToken("mixed");
    case Package::AssemblyType::Other:
      return SExpression::createToken("other");
    case Package::AssemblyType::Auto:
      return SExpression::createToken("auto");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline Package::AssemblyType deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("none")) {
    return Package::AssemblyType::None;
  } else if (str == QLatin1String("tht")) {
    return Package::AssemblyType::Tht;
  } else if (str == QLatin1String("smt")) {
    return Package::AssemblyType::Smt;
  } else if (str == QLatin1String("mixed")) {
    return Package::AssemblyType::Mixed;
  } else if (str == QLatin1String("other")) {
    return Package::AssemblyType::Other;
  } else if (str == QLatin1String("auto")) {
    return Package::AssemblyType::Auto;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown package assembly type: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Package::Package(const Uuid& uuid, const Version& version,
                 const QString& author, const ElementName& name_en_US,
                 const QString& description_en_US,
                 const QString& keywords_en_US, AssemblyType assemblyType)
  : LibraryElement(getShortElementName(), getLongElementName(), uuid, version,
                   author, name_en_US, description_en_US, keywords_en_US),
    mAssemblyType(assemblyType),
    mPads(),
    mFootprints() {
}

Package::Package(std::unique_ptr<TransactionalDirectory> directory,
                 const SExpression& root)
  : LibraryElement(getShortElementName(), getLongElementName(), true,
                   std::move(directory), root),
    mAssemblyType(deserialize<AssemblyType>(root.getChild("assembly_type/@0"))),
    mPads(root),
    mFootprints(root) {
}

Package::~Package() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Package::AssemblyType Package::getAssemblyType(bool resolveAuto) const
    noexcept {
  if (resolveAuto && (mAssemblyType == AssemblyType::Auto)) {
    return guessAssemblyType();
  } else {
    return mAssemblyType;
  }
}

Package::AssemblyType Package::guessAssemblyType() const noexcept {
  // Auto-detect based on default footprint pads.
  bool hasThtPads = false;
  bool hasSmtPads = false;
  if (!mFootprints.isEmpty()) {
    for (const FootprintPad& pad : mFootprints.first()->getPads()) {
      if (pad.isTht()) {
        hasThtPads = true;
      } else {
        hasSmtPads = true;
      }
    }
  }
  if (hasThtPads && hasSmtPads) {
    return AssemblyType::Mixed;
  } else if (hasThtPads) {
    return AssemblyType::Tht;
  } else if (hasSmtPads) {
    return AssemblyType::Smt;
  } else {
    return AssemblyType::None;
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList Package::runChecks() const {
  PackageCheck check(*this);
  return check.runChecks();  // can throw
}

std::unique_ptr<Package> Package::open(
    std::unique_ptr<TransactionalDirectory> directory) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  const auto migrations = FileFormatMigration::getMigrations(fileFormat);
  for (auto migration : migrations) {
    migration->upgradePackage(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const SExpression root = SExpression::parse(directory->read(fileName),
                                              directory->getAbsPath(fileName));
  std::unique_ptr<Package> obj(new Package(std::move(directory), root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
  }
  return obj;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Package::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("assembly_type", mAssemblyType);
  root.ensureLineBreak();
  mPads.serialize(root);
  root.ensureLineBreak();
  mFootprints.serialize(root);
  root.ensureLineBreak();
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
