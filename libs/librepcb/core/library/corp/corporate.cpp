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
#include "corporate.h"

#include "../../serialization/fileformatmigration.h"
#include "corporatecheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Corporate::Corporate(const Uuid& uuid, const Version& version,
                     const QString& author, const ElementName& name_en_US,
                     const QString& description_en_US,
                     const QString& keywords_en_US)
  : LibraryBaseElement(getShortElementName(), getLongElementName(), uuid,
                       version, author, name_en_US, description_en_US,
                       keywords_en_US) {
}

Corporate::Corporate(std::unique_ptr<TransactionalDirectory> directory,
                     const SExpression& root)
  : LibraryBaseElement(getShortElementName(), getLongElementName(), true,
                       std::move(directory), root),
    mIcon(),  // Initialized below.
    // Note: Don't use SExpression::getValueByPath<QUrl>() because it would
    // throw an exception if the URL is empty, which is actually legal in this
    // case.
    mUrl(root.getChild("url/@0").getValue(), QUrl::StrictMode)
{
  // Load image if available.
  mIcon = mDirectory->readIfExists("logo.png");  // can throw
}

Corporate::~Corporate() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QPixmap Corporate::getIconAsPixmap() const noexcept {
  QPixmap p;
  p.loadFromData(mIcon, "png");
  return p;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList Corporate::runChecks() const {
  CorporateCheck check(*this);
  return check.runChecks();  // can throw
}

void Corporate::save() {
  LibraryBaseElement::save();  // can throw

  // Save icon.
  if (mIcon.isEmpty()) {
    mDirectory->removeFile("logo.png");  // can throw
  } else if (!mIcon.isEmpty()) {
    mDirectory->write("logo.png", mIcon);  // can throw
  }
}

std::unique_ptr<Corporate> Corporate::open(
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
    migration->upgradeCorporate(*directory);
  }

  // Load element.
  const QString fileName = getLongElementName() % ".lp";
  const std::unique_ptr<const SExpression> root = SExpression::parse(
      directory->read(fileName), directory->getAbsPath(fileName));
  std::unique_ptr<Corporate> obj(new Corporate(std::move(directory), *root));
  if (!migrations.isEmpty()) {
    obj->removeObsoleteMessageApprovals();
    obj->save();  // Format all files correctly as the migration doesn't!
  }
  return obj;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void Corporate::serialize(SExpression& root) const {
  LibraryBaseElement::serialize(root);
  root.ensureLineBreak();
  root.appendChild("url", mUrl);
  root.ensureLineBreak();
  serializeMessageApprovals(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
