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
#include "librarycreator.h"

#include "../apptoolbox.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryCreator::LibraryCreator(Workspace& ws, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mName(parseElementName("My Library")),
    mDescription(),
    mAuthor(ws.getSettings().userName.get().trimmed()),
    mVersion(),
    mDirectory(),
    mFallbackDirectory() {
}

LibraryCreator::~LibraryCreator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QString LibraryCreator::setName(const QString& input) noexcept {
  mName = parseElementName(cleanElementName(input));
  validate();
  return mName ? QString() : "!";
}

QString LibraryCreator::setDescription(const QString& input) noexcept {
  mDescription = input.trimmed();
  validate();
  return QString();
}

QString LibraryCreator::setAuthor(const QString& input) noexcept {
  mAuthor = input.trimmed();
  validate();
  return QString();
}

QString LibraryCreator::setVersion(const QString& input) noexcept {
  mVersion = Version::tryFromString(input.trimmed());
  validate();
  return QString();
}

QString LibraryCreator::setDirectory(const QString& input,
                                     const QString& fallback) noexcept {
  auto parse = [](QString s) {
    s = cleanFileProofName(s);
    if (s.isEmpty() || (!s.endsWith(".lplib")) || s.startsWith(".")) {
      return std::optional<FileProofName>();
    }
    return parseFileProofName(s);
  };

  mDirectory = parse(input);
  mFallbackDirectory =
      input.trimmed().isEmpty() ? parse(fallback) : std::nullopt;
  validate();
  return (mDirectory || mFallbackDirectory)
      ? QString()
      : (tr("Example:") % " My_Library.lplib");
}

QString LibraryCreator::getDirectoryForName(const QString& input) noexcept {
  QString dir = FilePath::cleanFileName(
      input, FilePath::ReplaceSpaces | FilePath::KeepCase);
  if ((!dir.isEmpty()) && (!dir.endsWith(".lplib"))) dir.append(".lplib");
  return dir;
}

QString LibraryCreator::create() noexcept {
  return QString();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryCreator::validate() noexcept {
  bool valid = true;
  if (!mName) valid = false;
  if ((!mDirectory) && (!mFallbackDirectory)) valid = false;
  if (!mVersion) valid = false;
  emit validChanged(valid);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
