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
#include "filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FilePath::FilePath() noexcept : mIsValid(false), mFileInfo() {
  mFileInfo.setCaching(false);
}

FilePath::FilePath(const QString& filepath) noexcept
  : mIsValid(false), mFileInfo() {
  mFileInfo.setCaching(false);
  FilePath::setPath(filepath);
}

FilePath::FilePath(const FilePath& other) noexcept
  : mIsValid(other.mIsValid), mFileInfo(other.mFileInfo) {
  mFileInfo.setCaching(false);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool FilePath::setPath(const QString& filepath) noexcept {
  mFileInfo.setFile(makeWellFormatted(filepath));
  mIsValid = mFileInfo.isAbsolute();  // check if the filepath is absolute
  return mIsValid;
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool FilePath::isExistingFile() const noexcept {
  if (!mIsValid) return false;

  return (mFileInfo.isFile() && mFileInfo.exists());
}

bool FilePath::isExistingDir() const noexcept {
  if (!mIsValid) return false;

  return (mFileInfo.isDir() && mFileInfo.exists());
}

bool FilePath::isEmptyDir() const noexcept {
  if (!isExistingDir()) return false;

  QDir dir(mFileInfo.filePath());
  dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
  return (dir.count() == 0);
}

bool FilePath::isRoot() const noexcept {
  // do not use QFileInfo::isRoot() because it's not the same as QDir::isRoot()!
  QDir dir(mFileInfo.filePath());
  return mIsValid && dir.isRoot();
}

bool FilePath::isLocatedInDir(const FilePath& dir) const noexcept {
  if ((!mIsValid) || (!dir.mIsValid)) return false;

  return toStr().startsWith(dir.toStr() % "/", Qt::CaseInsensitive);
}

QString FilePath::toStr() const noexcept {
  if (!mIsValid) return QString();

  return mFileInfo.filePath();
}

QString FilePath::toNative() const noexcept {
  if (!mIsValid) return QString();

  return QDir::toNativeSeparators(mFileInfo.filePath());
}

FilePath FilePath::toUnique() const noexcept {
  if (!mIsValid) return FilePath();

  FilePath unique(mFileInfo.canonicalFilePath());

  if (!unique.isValid()) unique = *this;

  return unique;
}

QString FilePath::toRelative(const FilePath& base) const noexcept {
  if ((!mIsValid) || (!base.mIsValid)) return QString();

  QDir baseDir(base.mFileInfo.filePath());
  return makeWellFormatted(baseDir.relativeFilePath(mFileInfo.filePath()));
}

QString FilePath::getBasename() const noexcept {
  if (mIsValid)
    return mFileInfo.baseName();
  else
    return QString();
}

QString FilePath::getCompleteBasename() const noexcept {
  if (mIsValid)
    return mFileInfo.completeBaseName();
  else
    return QString();
}

QString FilePath::getSuffix() const noexcept {
  if (mIsValid)
    return mFileInfo.suffix();
  else
    return QString();
}

QString FilePath::getCompleteSuffix() const noexcept {
  if (mIsValid)
    return mFileInfo.completeSuffix();
  else
    return QString();
}

QString FilePath::getFilename() const noexcept {
  if (mIsValid)
    return mFileInfo.fileName();
  else
    return QString();
}

FilePath FilePath::getParentDir() const noexcept {
  if ((!mIsValid) || (isRoot())) return FilePath();

  return FilePath(mFileInfo.dir().path());
}

FilePath FilePath::getPathTo(const QString& filename) const noexcept {
  return FilePath(mFileInfo.filePath() % QLatin1Char('/') % filename);
}

/*******************************************************************************
 *  Operators
 ******************************************************************************/

FilePath& FilePath::operator=(const FilePath& rhs) noexcept {
  mFileInfo = rhs.mFileInfo;
  mIsValid = rhs.mIsValid;
  return *this;
}

bool FilePath::operator==(const FilePath& rhs) const noexcept {
  if (mIsValid != rhs.mIsValid) return false;
  if (mFileInfo.filePath() != rhs.mFileInfo.filePath()) return false;
  return true;
}

bool FilePath::operator!=(const FilePath& rhs) const noexcept {
  return !(*this == rhs);
}

bool FilePath::operator<(const FilePath& rhs) const noexcept {
  return toStr() < rhs.toStr();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

FilePath FilePath::fromRelative(const FilePath& base,
                                const QString& relative) noexcept {
  if (!base.mIsValid) return FilePath();

  return FilePath(base.mFileInfo.filePath() % QLatin1Char('/') % relative);
}

FilePath FilePath::getTempPath() noexcept {
  FilePath tmp(QDir::tempPath());

  if (!tmp.isExistingDir())
    qWarning() << "Could not determine the system's temporary directory!";

  return tmp;
}

FilePath FilePath::getApplicationTempPath() noexcept {
  return getTempPath().getPathTo("librepcb");
}

FilePath FilePath::getRandomTempPath() noexcept {
  QString random =
      QString("%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(qrand());
  return getApplicationTempPath().getPathTo(random);
}

QString FilePath::cleanFileName(const QString& userInput,
                                CleanFileNameOptions options) noexcept {
  // perform compatibility decomposition (NFKD)
  QString ret = userInput.normalized(QString::NormalizationForm_KD);
  // remove all invalid characters
  ret.remove(QRegularExpression("[^-._ 0-9A-Za-z]"));
  // remove leading and trailing spaces
  ret = ret.trimmed();
  // replace remaining spaces with underscore (if corresponding option set)
  if (options.testFlag(CleanFileNameOption::ReplaceSpaces))
    ret.replace(' ', '_');
  // change case of all characters (if corresponding options set)
  if (options.testFlag(CleanFileNameOption::ToLowerCase)) ret = ret.toLower();
  if (options.testFlag(CleanFileNameOption::ToUpperCase)) ret = ret.toUpper();
  // limit length of string to a reasonable number of characters
  ret.truncate(120);
  return ret;
}

QString FilePath::makeWellFormatted(const QString& filepath) noexcept {
  // change all separators to "/", remove redundant separators, resolve "." and
  // "..".
  QString newPath = QDir::cleanPath(filepath);

  // tests show that QDir::cleanPath() will also remove a slash at the end of
  // the filepath, but as this feature is not described in the documentation, we
  // will do this again to ensure that this will always work correctly!
  while ((newPath.endsWith("/")) &&
         (newPath != "/"))  // the last character is "/"
    newPath.chop(1);  // remove the last character

  // Make sure that Windows drive paths end with a slash (i.e. convert "C:" to
  // "C:/"). This is important for FilePath::isRoot() to work properly (it fails
  // with "C:")!
  if ((newPath.length() == 2) && newPath.endsWith(":")) {
    newPath += '/';
  }

  // convert "." (current directory) to "" (especially required for Qt >= 5.5.1
  // as QDir::relativeFilePath() now returns a dot instead of an empty string)
  if (newPath == ".") newPath = QString("");

  return newPath;
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

QDataStream& operator<<(QDataStream& stream, const FilePath& filepath) {
  stream << filepath.toStr();
  return stream;
}

QDebug& operator<<(QDebug& stream, const FilePath& filepath) {
  stream << QString("FilePath(%1)").arg(filepath.toStr());
  return stream;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
