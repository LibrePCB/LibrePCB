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
#include "translationscatalog.h"

#include "../fileio/fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TranslationsCatalog::TranslationsCatalog() noexcept {
}

TranslationsCatalog::~TranslationsCatalog() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void TranslationsCatalog::saveTo(const FilePath& fp) const {
  auto escape = [](QString s) {
    return s.replace("\"", "'").replace("\n", "\\n").replace("\r", "");
  };

  QStringList lines;
  for (auto it = mStrings.begin(); it != mStrings.end(); it++) {
    lines.append(QString("#: %1").arg(it->location));
    lines.append(QString("#. %1").arg(it->comment.simplified()));
    lines.append(QString("msgid \"%1\"").arg(escape(it->id)));
    lines.append(QString("msgstr \"%1\"").arg(escape(it->string)));
    lines.append(QString());
  }
  FileUtils::writeFile(fp, lines.join("\n").toUtf8());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
