/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "strokefontpool.h"

#include "../fileio/fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StrokeFontPool::StrokeFontPool(const FilePath& directory) noexcept {
  try {
    foreach (const FilePath& fp,
             FileUtils::getFilesInDirectory(directory, {"*.bene"})) {
      try {
        qDebug() << "Load stroke font:" << fp.getFilename();
        mFonts.insert(fp.getFilename(),
                      std::make_shared<StrokeFont>(fp));  // can throw
      } catch (const Exception& e) {
        qCritical() << "Failed to load stroke font" << fp.toNative() << ":"
                    << e.getMsg();
      }
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to load stroke font pool:" << e.getMsg();
  }
}

StrokeFontPool::~StrokeFontPool() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const StrokeFont& StrokeFontPool::getFont(const QString& filename) const {
  if (mFonts.contains(filename)) {
    return *mFonts[filename];
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("The font \"%1\" does not exist in the font pool."))
            .arg(filename));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
