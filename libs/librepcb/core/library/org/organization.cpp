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
#include "organization.h"

#include "../../serialization/fileformatmigration.h"
#include "organizationcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Organization::Organization(const Uuid& uuid, const Version& version,
                           const QString& author, const ElementName& name_en_US,
                           const QString& description_en_US,
                           const QString& keywords_en_US)
  : LibraryBaseElement(getShortElementName(), getLongElementName(), uuid,
                       version, author, name_en_US, description_en_US,
                       keywords_en_US),
    mLogoPng(),
    mUrl(),
    mCountry(),
    mFabs(),
    mShipping(),
    mIsSponsor(false),
    mPriority(0),
    mPcbDesignRules(),
    mPcbOutputJobs(),
    mAssemblyOutputJobs(),
    mUserOutputJobs(),
    mOptions() {
}

Organization::Organization(std::unique_ptr<TransactionalDirectory> directory,
                           const SExpression& root)
  : LibraryBaseElement(getShortElementName(), getLongElementName(), true,
                       std::move(directory), root),
    mLogoPng(),  // Initialized below.
    // Note: Don't use SExpression::getValueByPath<QUrl>() because it would
    // throw an exception if the URL is empty, which is actually legal in this
    // case.
    mUrl(root.getChild("url/@0").getValue(), QUrl::StrictMode),
    mCountry(root.getChild("country/@0").getValue()),
    mFabs(root.getChild("fabs/@0").getValue().split(",")),
    mShipping(root.getChild("shipping/@0").getValue().split(",")),
    mIsSponsor(deserialize<bool>(root.getChild("sponsor/@0"))),
    mPriority(deserialize<int>(root.getChild("priority/@0"))),
    mPcbDesignRules(),  // Initialized below.
    mPcbOutputJobs(),  // Initialized below.
    mAssemblyOutputJobs(),  // Initialized below.
    mUserOutputJobs(),  // Initialized below.
    mOptions()  // Initialized below.
{
  // Load image if available.
  mLogoPng = mDirectory->readIfExists("logo.png");  // can throw

  foreach (const SExpression* child, root.getChildren("pcb_design_rules")) {
    mPcbDesignRules.append(OrganizationPcbDesignRules(*child));  // can throw
  }
  foreach (const SExpression* child, root.getChildren("pcb_job")) {
    mPcbOutputJobs.append(
        deserialize<std::shared_ptr<OutputJob>>(*child));  // can throw
  }
  foreach (const SExpression* child, root.getChildren("assembly_job")) {
    mAssemblyOutputJobs.append(
        deserialize<std::shared_ptr<OutputJob>>(*child));  // can throw
  }
  foreach (const SExpression* child, root.getChildren("user_job")) {
    mUserOutputJobs.append(
        deserialize<std::shared_ptr<OutputJob>>(*child));  // can throw
  }
  foreach (const SExpression* child, root.getChildren("option")) {
    mOptions[child->getChild("@0").getValue()].append(*child);
  }
}

Organization::~Organization() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QPixmap Organization::getLogoPixmap() const noexcept {
  QPixmap p;
  p.loadFromData(mLogoPng, "png");
  return p;
}

const OrganizationPcbDesignRules* Organization::findPcbDesignRules(
    const Uuid& uuid) const noexcept {
  for (const auto& obj : mPcbDesignRules) {
    if (obj.getUuid() == uuid) {
      return &obj;
    }
  }
  return nullptr;
}

std::shared_ptr<const OutputJob> Organization::findPcbOutputJob(
    const QString& type) const noexcept {
  for (int i = 0; i < mPcbOutputJobs.count(); ++i) {
    if (mPcbOutputJobs.at(i)->getType() == type) {
      return mPcbOutputJobs.at(i);
    }
  }
  return nullptr;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Organization::setPcbDesignRules(
    const QVector<OrganizationPcbDesignRules>& rules) noexcept {
  if (rules != mPcbDesignRules) {
    mPcbDesignRules = rules;
    emit pcbDesignRulesModified();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList Organization::runChecks() const {
  OrganizationCheck check(*this);
  return check.runChecks();  // can throw
}

void Organization::save() {
  LibraryBaseElement::save();  // can throw

  // Save icon.
  if (mLogoPng.isEmpty()) {
    mDirectory->removeFile("logo.png");  // can throw
  } else if (!mLogoPng.isEmpty()) {
    mDirectory->write("logo.png", mLogoPng);  // can throw
  }
}

std::unique_ptr<Organization> Organization::open(
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
    migration->upgradeOrganization(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const std::unique_ptr<const SExpression> root = SExpression::parse(
      directory->read(fileName), directory->getAbsPath(fileName));
  std::unique_ptr<Organization> obj(
      new Organization(std::move(directory), *root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
    obj->save();  // Format all files correctly as the migration doesn't!
  }
  return obj;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Organization::serialize(SExpression& root) const {
  LibraryBaseElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("url", mUrl);
  root.ensureLineBreak();
  root.appendChild("country", mCountry);
  root.ensureLineBreak();
  root.appendChild("fabs", mFabs.join(","));
  root.ensureLineBreak();
  root.appendChild("shipping", mShipping.join(","));
  root.ensureLineBreak();
  root.appendChild("sponsor", mIsSponsor);
  root.ensureLineBreak();
  root.appendChild("priority", mPriority);
  root.ensureLineBreak();
  for (const OrganizationPcbDesignRules& rules : mPcbDesignRules) {
    rules.serialize(root.appendList("pcb_design_rules"));
    root.ensureLineBreak();
  }
  for (const auto& job : mPcbOutputJobs) {
    job.serialize(root.appendList("pcb_job"));
    root.ensureLineBreak();
  }
  for (const auto& job : mAssemblyOutputJobs) {
    job.serialize(root.appendList("assembly_job"));
    root.ensureLineBreak();
  }
  for (const auto& job : mUserOutputJobs) {
    job.serialize(root.appendList("user_job"));
    root.ensureLineBreak();
  }
  foreach (const auto& list, mOptions) {
    foreach (const auto& node, list) {
      root.appendChild(node);
      root.ensureLineBreak();
    }
  }
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
