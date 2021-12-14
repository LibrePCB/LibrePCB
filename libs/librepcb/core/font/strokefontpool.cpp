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
#include "strokefontpool.h"

#include "../fileio/filesystem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StrokeFontPool::StrokeFontPool(const FileSystem& directory) noexcept {
  foreach (const QString& filename, directory.getFiles()) {
    FilePath fp = directory.getAbsPath(filename);
    if (fp.getSuffix() != "bene") continue;
    try {
      qDebug() << "Load stroke font:" << filename;
      mFonts.insert(
          filename,
          std::make_shared<StrokeFont>(fp,
                                       directory.read(filename)));  // can throw
    } catch (const Exception& e) {
      qCritical() << "Failed to load stroke font" << fp.toNative() << ":"
                  << e.getMsg();
    }
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
        tr("The font \"%1\" does not exist in the font pool.").arg(filename));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
