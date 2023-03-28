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
#include "librarybaseelement.h"

#include "../application.h"
#include "../fileio/versionfile.h"
#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"
#include "librarybaseelementcheck.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryBaseElement::LibraryBaseElement(const QString& shortElementName,
                                       const QString& longElementName,
                                       const Uuid& uuid, const Version& version,
                                       const QString& author,
                                       const ElementName& name_en_US,
                                       const QString& description_en_US,
                                       const QString& keywords_en_US)
  : QObject(nullptr),
    mShortElementName(shortElementName),
    mLongElementName(longElementName),
    mDirectory(new TransactionalDirectory()),
    mUuid(uuid),
    mVersion(version),
    mAuthor(author),
    mCreated(QDateTime::currentDateTime()),
    mIsDeprecated(false),
    mNames(name_en_US),
    mDescriptions(description_en_US),
    mKeywords(keywords_en_US),
    mMessageApprovals() {
}

LibraryBaseElement::LibraryBaseElement(
    const QString& shortElementName, const QString& longElementName,
    bool dirnameMustBeUuid, std::unique_ptr<TransactionalDirectory> directory,
    const SExpression& root)
  : QObject(nullptr),
    mShortElementName(shortElementName),
    mLongElementName(longElementName),
    mDirectory(std::move(directory)),
    mUuid(deserialize<Uuid>(root.getChild("@0"))),
    mVersion(deserialize<Version>(root.getChild("version/@0"))),
    mAuthor(root.getChild("author/@0").getValue()),
    mCreated(deserialize<QDateTime>(root.getChild("created/@0"))),
    mIsDeprecated(deserialize<bool>(root.getChild("deprecated/@0"))),
    mNames(root),
    mDescriptions(root),
    mKeywords(root),
    mMessageApprovals() {
  // Load message approvals.
  foreach (const SExpression* child, root.getChildren("approved")) {
    mMessageApprovals.insert(*child);
  }

  // Check directory name.
  const QString dirName = mDirectory->getAbsPath().getFilename();
  if (dirnameMustBeUuid && (dirName != mUuid.toStr())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Directory name UUID mismatch: '%1' != '%2'\n\nDirectory: '%3'")
            .arg(dirName, mUuid.toStr(), mDirectory->getAbsPath().toNative()));
  }
}

LibraryBaseElement::~LibraryBaseElement() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QStringList LibraryBaseElement::getAllAvailableLocales() const noexcept {
  QStringList list;
  list.append(mNames.keys());
  list.append(mDescriptions.keys());
  list.append(mKeywords.keys());
  list.removeDuplicates();
  list.sort(Qt::CaseSensitive);
  return list;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList LibraryBaseElement::runChecks() const {
  LibraryBaseElementCheck check(*this);
  return check.runChecks();  // can throw
}

void LibraryBaseElement::save() {
  // Content.
  SExpression root = SExpression::createList("librepcb_" % mLongElementName);
  serialize(root);
  mDirectory->write(mLongElementName % ".lp", root.toByteArray());

  // Version file.
  mDirectory->write(
      ".librepcb-" % mShortElementName,
      VersionFile(Application::getFileFormatVersion()).toByteArray());
}

void LibraryBaseElement::saveTo(TransactionalDirectory& dest) {
  mDirectory->saveTo(dest);  // can throw
  save();  // can throw
}

void LibraryBaseElement::moveTo(TransactionalDirectory& dest) {
  mDirectory->moveTo(dest);  // can throw
  save();  // can throw
}

void LibraryBaseElement::saveIntoParentDirectory(TransactionalDirectory& dest) {
  TransactionalDirectory dir(dest, mUuid.toStr());
  saveTo(dir);  // can throw
}

void LibraryBaseElement::moveIntoParentDirectory(TransactionalDirectory& dest) {
  TransactionalDirectory dir(dest, mUuid.toStr());
  moveTo(dir);  // can throw
}

void LibraryBaseElement::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  mNames.serialize(root);
  root.ensureLineBreak();
  mDescriptions.serialize(root);
  root.ensureLineBreak();
  mKeywords.serialize(root);
  root.ensureLineBreak();
  root.appendChild("author", mAuthor);
  root.ensureLineBreak();
  root.appendChild("version", mVersion);
  root.ensureLineBreak();
  root.appendChild("created", mCreated);
  root.ensureLineBreak();
  root.appendChild("deprecated", mIsDeprecated);
  root.ensureLineBreak();
}

void LibraryBaseElement::serializeMessageApprovals(SExpression& root) const {
  foreach (const SExpression& node, Toolbox::sortedQSet(mMessageApprovals)) {
    root.ensureLineBreak();
    root.appendChild(node);
  }
  root.ensureLineBreak();
}

void LibraryBaseElement::removeObsoleteMessageApprovals() {
  const RuleCheckMessageList messages = runChecks();
  mMessageApprovals &= RuleCheckMessage::getAllApprovals(messages);
}

Version LibraryBaseElement::readFileFormat(
    const TransactionalDirectory& directory, const QString& fileName) {
  const VersionFile versionFile =
      VersionFile::fromByteArray(directory.read(fileName));
  const Version fileFormat = versionFile.getVersion();
  if (fileFormat > Application::getFileFormatVersion()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("This library element was created with a newer "
                          "application version.\n"
                          "You need at least LibrePCB %1 to open it.\n\n%2")
                           .arg(fileFormat.toPrettyStr(3))
                           .arg(directory.getAbsPath().toNative()));
  }
  return fileFormat;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
