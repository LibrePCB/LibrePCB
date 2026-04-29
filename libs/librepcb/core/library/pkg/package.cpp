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
std::unique_ptr<SExpression> serialize(const Package::AssemblyType& obj) {
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
                 const QString& author, const QDateTime& created,
                 const ElementName& name_en_US,
                 const QString& description_en_US,
                 const QString& keywords_en_US, AssemblyType assemblyType)
  : LibraryElement(getShortElementName(), getLongElementName(), uuid, version,
                   author, created, name_en_US, description_en_US,
                   keywords_en_US),
    mAlternativeNames(),
    mAssemblyType(assemblyType),
    mGridInterval(2540000),
    mMinCopperClearance(200000),  // 200 µm
    mPads(),
    mModels(),
    mFootprints() {
}

Package::Package(std::unique_ptr<TransactionalDirectory> directory,
                 const SExpression& root)
  : LibraryElement(getShortElementName(), getLongElementName(), true,
                   std::move(directory), root),
    mAlternativeNames(),
    mAssemblyType(deserialize<AssemblyType>(root.getChild("assembly_type/@0"))),
    mGridInterval(
        deserialize<PositiveLength>(root.getChild("grid_interval/@0"))),
    mMinCopperClearance(
        deserialize<UnsignedLength>(root.getChild("min_copper_clearance/@0"))),
    mPads(root),
    mModels(root),
    mFootprints(root) {
  foreach (const SExpression* node, root.getChildren("alternative_name")) {
    mAlternativeNames.append(AlternativeName(*node));
  }
}

Package::~Package() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Package::AssemblyType Package::getAssemblyType(
    bool resolveAuto) const noexcept {
  if (resolveAuto && (mAssemblyType == AssemblyType::Auto)) {
    return guessAssemblyType();
  } else {
    return mAssemblyType;
  }
}

Package::AssemblyType Package::guessAssemblyType() const noexcept {
  // If there are no package pads, probably there's nothing to mount.
  if (mPads.isEmpty()) {
    return AssemblyType::None;
  }

  // Auto-detect based on default footprint pads.
  bool hasThtPads = false;
  bool hasSmtPads = false;
  if (!mFootprints.isEmpty()) {
    for (const FootprintPad& pad : mFootprints.first()->getPads()) {
      if (pad.getFunctionNeedsSoldering()) {
        if (pad.isTht()) {
          hasThtPads = true;
        } else {
          hasSmtPads = true;
        }
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

QVector<std::shared_ptr<const PackageModel>> Package::getModelsForFootprint(
    const Uuid& fpt) const noexcept {
  QVector<std::shared_ptr<const PackageModel>> result;
  if (std::shared_ptr<const Footprint> footprint = mFootprints.find(fpt)) {
    for (auto modelIt = mModels.begin(); modelIt != mModels.end(); ++modelIt) {
      if (footprint->getModels().contains(modelIt->getUuid())) {
        result.append(modelIt.ptr());
      }
    }
  }
  return result;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Package::duplicateFrom(const Package& other) {
  LibraryElement::duplicateFrom(other);
  setAlternativeNames(other.getAlternativeNames());
  setAssemblyType(other.getAssemblyType(false));
  setGridInterval(other.getGridInterval());
  setMinCopperClearance(other.getMinCopperClearance());

  // Copy pads but generate new UUIDs.
  mPads.clear();
  QHash<Uuid, std::optional<Uuid>> padUuidMap;
  for (const PackagePad& pad : other.getPads()) {
    const Uuid newUuid = Uuid::createRandom();
    padUuidMap.insert(pad.getUuid(), newUuid);
    mPads.append(std::make_shared<PackagePad>(newUuid, pad.getName()));
  }

  // Copy 3D models but generate new UUIDs.
  mModels.clear();
  QHash<Uuid, std::optional<Uuid>> modelsUuidMap;
  for (const PackageModel& model : other.getModels()) {
    auto newModel =
        std::make_shared<PackageModel>(Uuid::createRandom(), model.getName());
    modelsUuidMap.insert(model.getUuid(), newModel->getUuid());
    mModels.append(newModel);
    const QByteArray fileContent =
        other.getDirectory().readIfExists(model.getFileName());
    if (!fileContent.isNull()) {
      mDirectory->write(newModel->getFileName(), fileContent);
    }
  }

  // Copy footprints but generate new UUIDs.
  mFootprints.clear();
  for (const Footprint& footprint : other.getFootprints()) {
    // Don't copy translations as they would need to be adjusted anyway.
    std::shared_ptr<Footprint> newFootprint(new Footprint(
        Uuid::createRandom(), footprint.getNames().getDefaultValue(),
        footprint.getDescriptions().getDefaultValue()));
    newFootprint->setModelPosition(footprint.getModelPosition());
    newFootprint->setModelRotation(footprint.getModelRotation());
    // Copy models but with the new UUIDs.
    QSet<Uuid> models;
    foreach (const Uuid& uuid, footprint.getModels()) {
      if (auto newUuid = modelsUuidMap.value(uuid)) {
        models.insert(*newUuid);
      }
    }
    newFootprint->setModels(models);
    // Copy pads but generate new UUIDs.
    for (const FootprintPad& pad : footprint.getPads()) {
      std::optional<Uuid> pkgPad = pad.getPackagePadUuid();
      if (pkgPad) {
        pkgPad = padUuidMap.value(*pkgPad);  // Translate to new UUID
      }
      newFootprint->getPads().append(std::make_shared<FootprintPad>(
          Uuid::createRandom(), pkgPad, pad.getPosition(), pad.getRotation(),
          pad.getShape(), pad.getWidth(), pad.getHeight(), pad.getRadius(),
          pad.getCustomShapeOutline(), pad.getStopMaskConfig(),
          pad.getSolderPasteConfig(), pad.getCopperClearance(),
          pad.getComponentSide(), pad.getFunction(), pad.getHoles()));
    }
    // Copy polygons but generate new UUIDs.
    for (const Polygon& polygon : footprint.getPolygons()) {
      newFootprint->getPolygons().append(std::make_shared<Polygon>(
          Uuid::createRandom(), polygon.getLayer(), polygon.getLineWidth(),
          polygon.isFilled(), polygon.isGrabArea(), polygon.getPath()));
    }
    // Copy circles but generate new UUIDs.
    for (const Circle& circle : footprint.getCircles()) {
      newFootprint->getCircles().append(std::make_shared<Circle>(
          Uuid::createRandom(), circle.getLayer(), circle.getLineWidth(),
          circle.isFilled(), circle.isGrabArea(), circle.getCenter(),
          circle.getDiameter()));
    }
    // Copy stroke texts but generate new UUIDs.
    for (const StrokeText& text : footprint.getStrokeTexts()) {
      newFootprint->getStrokeTexts().append(std::make_shared<StrokeText>(
          Uuid::createRandom(), text.getLayer(), text.getText(),
          text.getPosition(), text.getRotation(), text.getHeight(),
          text.getStrokeWidth(), text.getLetterSpacing(), text.getLineSpacing(),
          text.getAlign(), text.getMirrored(), text.getAutoRotate(),
          text.isLocked()));
    }
    // Copy zones but generate new UUIDs.
    for (const Zone& zone : footprint.getZones()) {
      newFootprint->getZones().append(
          std::make_shared<Zone>(Uuid::createRandom(), zone));
    }
    // Copy holes but generate new UUIDs.
    for (const Hole& hole : footprint.getHoles()) {
      newFootprint->getHoles().append(
          std::make_shared<Hole>(Uuid::createRandom(), hole.getDiameter(),
                                 hole.getPath(), hole.getStopMaskConfig()));
    }
    mFootprints.append(newFootprint);
  }
}

RuleCheckMessageList Package::runChecks() const {
  PackageCheck check(*this);
  return check.runChecks();  // can throw
}

std::unique_ptr<Package> Package::open(
    std::unique_ptr<TransactionalDirectory> directory,
    bool abortBeforeMigration) {
  Q_ASSERT(directory);

  // Upgrade file format, if needed.
  const Version fileFormat =
      readFileFormat(*directory, ".librepcb-" % getShortElementName());
  const auto migrations = FileFormatMigration::getMigrations(fileFormat);
  if (abortBeforeMigration && (!migrations.isEmpty())) {
    return nullptr;
  }
  for (auto migration : migrations) {
    migration->upgradePackage(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const std::unique_ptr<const SExpression> root = SExpression::parse(
      directory->read(fileName), directory->getAbsPath(fileName));
  std::unique_ptr<Package> obj(new Package(std::move(directory), *root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
    obj->save();  // Format all files correctly as the migration doesn't!
  }
  return obj;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Package::serialize(SExpression& root) const {
  LibraryElement::serialize(root);
  foreach (const AlternativeName& name, mAlternativeNames) {
    root.ensureLineBreak();
    name.serialize(root.appendList("alternative_name"));
  }
  root.ensureLineBreak();
  root.appendChild("assembly_type", mAssemblyType);
  root.ensureLineBreak();
  root.appendChild("grid_interval", mGridInterval);
  root.ensureLineBreak();
  root.appendChild("min_copper_clearance", mMinCopperClearance);
  root.ensureLineBreak();
  mPads.serialize(root);
  root.ensureLineBreak();
  mModels.serialize(root);
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
