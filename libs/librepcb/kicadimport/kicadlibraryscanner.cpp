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
#include "kicadlibraryscanner.h"

#include "kicadtypes.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace kicadimport {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryScanner::KiCadLibraryScanner(QObject* parent) noexcept
  : QObject(parent), mLogger(new MessageLogger()), mAbort(false) {
}

KiCadLibraryScanner::~KiCadLibraryScanner() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryScanner::startScan(const FilePath& dir) {
  cancel();

#if (QT_VERSION_MAJOR >= 6)
  mFuture = QtConcurrent::run(&KiCadLibraryScanner::run, this, dir);
#else
  mFuture = QtConcurrent::run(this, &GraphicsExport::run, args);
#endif
}

std::shared_ptr<KiCadLibraryScanner::Result> KiCadLibraryScanner::getResult() noexcept {
  mFuture.waitForFinished();
  return mFuture.result();
}

void KiCadLibraryScanner::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/



/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
