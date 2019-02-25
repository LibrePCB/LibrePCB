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

#include "librarybaseelementcheck.h"

#include <librepcb/common/application.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/smartversionfile.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryBaseElement::LibraryBaseElement(
    bool dirnameMustBeUuid, const QString& shortElementName,
    const QString& longElementName, const Uuid& uuid, const Version& version,
    const QString& author, const ElementName& name_en_US,
    const QString& description_en_US, const QString& keywords_en_US)
  : QObject(nullptr),
    mDirectory(FilePath::getRandomTempPath()),
    mDirectoryIsTemporary(true),
    mOpenedReadOnly(false),
    mDirectoryNameMustBeUuid(dirnameMustBeUuid),
    mShortElementName(shortElementName),
    mLongElementName(longElementName),
    mUuid(uuid),
    mVersion(version),
    mAuthor(author),
    mCreated(QDateTime::currentDateTime()),
    mIsDeprecated(false),
    mNames(name_en_US),
    mDescriptions(description_en_US),
    mKeywords(keywords_en_US) {
  FileUtils::makePath(mDirectory);  // can throw
}

LibraryBaseElement::LibraryBaseElement(const FilePath& elementDirectory,
                                       bool            dirnameMustBeUuid,
                                       const QString&  shortElementName,
                                       const QString&  longElementName,
                                       bool            readOnly)
  : QObject(nullptr),
    mDirectory(elementDirectory),
    mDirectoryIsTemporary(false),
    mOpenedReadOnly(readOnly),
    mDirectoryNameMustBeUuid(dirnameMustBeUuid),
    mShortElementName(shortElementName),
    mLongElementName(longElementName),
    mUuid(Uuid::createRandom()),  // just for initialization, will be
                                  // overwritten
    mVersion(Version::fromString(
        "0.1")),  // just for initialization, will be overwritten
    mNames(ElementName(
        "unknown")),  // just for initialization, will be overwritten
    mDescriptions(""),
    mKeywords("") {
  // determine the filepath to the version file
  FilePath versionFilePath =
      mDirectory.getPathTo(".librepcb-" % mShortElementName);

  // check if the directory is a library element
  if (!versionFilePath.isExistingFile()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Directory is not a library element of type %1: \"%2\""))
            .arg(mLongElementName, mDirectory.toNative()));
  }

  // check directory name
  QString dirUuidStr = mDirectory.getFilename();
  if (mDirectoryNameMustBeUuid && (!Uuid::isValid(dirUuidStr))) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString(tr("Directory name is not a valid UUID: \"%1\""))
                           .arg(mDirectory.toNative()));
  }

  // read version number from version file
  SmartVersionFile versionFile(versionFilePath, false, true);
  if (versionFile.getVersion() > qApp->getAppVersion()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(
            tr("The library element %1 was created with a newer application "
               "version. You need at least LibrePCB version %2 to open it."))
            .arg(mDirectory.toNative())
            .arg(versionFile.getVersion().toPrettyStr(3)));
  }

  // open main file
  FilePath       sexprFilePath = mDirectory.getPathTo(mLongElementName % ".lp");
  SmartSExprFile sexprFile(sexprFilePath, false, true);
  mLoadingFileDocument = sexprFile.parseFileAndBuildDomTree();

  // read attributes
  mUuid         = mLoadingFileDocument.getChildByIndex(0).getValue<Uuid>();
  mVersion      = mLoadingFileDocument.getValueByPath<Version>("version");
  mAuthor       = mLoadingFileDocument.getValueByPath<QString>("author");
  mCreated      = mLoadingFileDocument.getValueByPath<QDateTime>("created");
  mIsDeprecated = mLoadingFileDocument.getValueByPath<bool>("deprecated");

  // read names, descriptions and keywords in all available languages
  mNames        = LocalizedNameMap(mLoadingFileDocument);
  mDescriptions = LocalizedDescriptionMap(mLoadingFileDocument);
  mKeywords     = LocalizedKeywordsMap(mLoadingFileDocument);

  // check if the UUID equals to the directory basename
  if (mDirectoryNameMustBeUuid && (mUuid.toStr() != dirUuidStr)) {
    qDebug() << mUuid.toStr() << "!=" << dirUuidStr;
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(
            tr("UUID mismatch between element directory and main file: \"%1\""))
            .arg(sexprFilePath.toNative()));
  }
}

LibraryBaseElement::~LibraryBaseElement() noexcept {
  if (mDirectoryIsTemporary) {
    if (!QDir(mDirectory.toStr()).removeRecursively()) {
      qWarning() << "Could not remove temporary directory:"
                 << mDirectory.toNative();
    }
  }
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

LibraryElementCheckMessageList LibraryBaseElement::runChecks() const {
  LibraryBaseElementCheck check(*this);
  return check.runChecks();  // can throw
}

void LibraryBaseElement::save() {
  if (mOpenedReadOnly) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Library element was opened in read-only mode: \"%1\""))
            .arg(mDirectory.toNative()));
  }

  // save S-Expressions file
  FilePath    sexprFilePath = mDirectory.getPathTo(mLongElementName % ".lp");
  SExpression root(serializeToDomElement("librepcb_" % mLongElementName));
  QScopedPointer<SmartSExprFile> sexprFile(
      SmartSExprFile::create(sexprFilePath));
  sexprFile->save(root, true);

  // save version number file
  QScopedPointer<SmartVersionFile> versionFile(SmartVersionFile::create(
      mDirectory.getPathTo(".librepcb-" % mShortElementName),
      qApp->getFileFormatVersion()));
  versionFile->save(true);
}

void LibraryBaseElement::saveTo(const FilePath& destination) {
  // copy to new directory and remove source directory if it was temporary
  copyTo(destination, mDirectoryIsTemporary);
}

void LibraryBaseElement::saveIntoParentDirectory(const FilePath& parentDir) {
  FilePath elemDir = parentDir.getPathTo(mUuid.toStr());
  saveTo(elemDir);
}

void LibraryBaseElement::moveTo(const FilePath& destination) {
  // copy to new directory and remove source directory
  copyTo(destination, true);
}

void LibraryBaseElement::moveIntoParentDirectory(const FilePath& parentDir) {
  FilePath elemDir = parentDir.getPathTo(mUuid.toStr());
  moveTo(elemDir);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void LibraryBaseElement::cleanupAfterLoadingElementFromFile() noexcept {
  mLoadingFileDocument = SExpression();  // destroy the whole DOM tree
}

void LibraryBaseElement::copyTo(const FilePath& destination,
                                bool            removeSource) {
  if (destination != mDirectory) {
    // check destination directory name validity
    if (mDirectoryNameMustBeUuid &&
        (destination.getFilename() != mUuid.toStr())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString(
              tr("Library element directory name is not a valid UUID: \"%1\""))
              .arg(destination.getFilename()));
    }

    // Unfortunately empty directories are sometimes not removed properly, for
    // example some Git operations remove the contained files but not the parent
    // directories. But if the destination directory exists already,
    // FileUtils::copyDirRecursively() would throw an exception because it
    // requires that the destination does not exist yet. To avoid this issue, we
    // remove the destination first if it's an empty directory. If it's not
    // empty, we accept that an exception is thrown because this would likely be
    // a serious error.
    if (destination.isEmptyDir()) {
      FileUtils::removeDirRecursively(destination);
    }

    // check if destination directory exists already
    if (destination.isExistingDir() || destination.isExistingFile()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString(tr("Could not copy "
                     "library element \"%1\" to \"%2\" because the destination "
                     "exists already."))
              .arg(mDirectory.toNative(), destination.toNative()));
    }

    // copy current directory to destination
    FileUtils::copyDirRecursively(mDirectory, destination);

    // memorize the current directory
    FilePath sourceDir = mDirectory;

    // save the library element to the destination directory
    mDirectory            = destination;
    mDirectoryIsTemporary = false;
    mOpenedReadOnly       = false;
    save();

    // remove source directory if required
    if (removeSource) {
      FileUtils::removeDirRecursively(sourceDir);
    }
  } else {
    // no copy action required, just save the element
    save();
  }
}

void LibraryBaseElement::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  mNames.serialize(root);
  mDescriptions.serialize(root);
  mKeywords.serialize(root);
  root.appendChild("author", mAuthor, true);
  root.appendChild("version", mVersion, true);
  root.appendChild("created", mCreated, true);
  root.appendChild("deprecated", mIsDeprecated, true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
