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
#include "kicadlibraryimport.h"

#include "kicadlibraryconverter.h"
#include "kicadtypeconverter.h"
#include "kicadtypes.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/utils/messagelogger.h>
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

KiCadLibraryImport::KiCadLibraryImport(const FilePath& dstLibFp,
                                       QObject* parent) noexcept
  : QObject(parent),
    mDestinationLibraryFp(dstLibFp),
    mLogger(new MessageLogger(true)),
    mAbort(false) {
}

KiCadLibraryImport::~KiCadLibraryImport() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImport::reset() noexcept {
  cancel();
  mState = State::Reset;
}

void KiCadLibraryImport::startScan(const FilePath& dir, MessageLogger& log) {
  if (mState != State::Reset) {throw LogicError(__FILE__, __LINE__, "Unexpected state.");}

#if (QT_VERSION_MAJOR >= 6)
  mFuture = QtConcurrent::run(&KiCadLibraryImport::scan, this, dir);
#else
  mFuture = QtConcurrent::run(this, &GraphicsExport::run, args);
#endif
}

void KiCadLibraryImport::startParse(MessageLogger& log) {

}

void KiCadLibraryImport::startImport() {

}

std::shared_ptr<KiCadLibraryImport::Result> KiCadLibraryImport::getResult() noexcept {
  mFuture.waitForFinished();
  return mFuture.result();
}

void KiCadLibraryImport::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

std::shared_ptr<KiCadLibraryImport::Result> KiCadLibraryImport::scan(FilePath dir) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Start graphics export in worker thread...";
  emit progressPercent(10);

  // Helper to find files or directories in a directory.
  auto findItems = [](const FilePath& dir, QDir::Filter filter,
                      const QString& pattern) {
    QList<FilePath> files;
    QDir qDir(dir.toStr());
    qDir.setFilter(filter | QDir::NoDotAndDotDot);
    qDir.setNameFilters({pattern});
    foreach (const QFileInfo& info, qDir.entryInfoList()) {
      files.append(FilePath{info.absoluteFilePath()});
    }
    return files;
  };

  // Helper to find libraries in a directory.
  auto result = std::make_shared<Result>();
  auto findLibs = [&](const FilePath& dir) {
    for (const FilePath& fp : findItems(dir, QDir::Dirs, "*.pretty")) {
      result->footprintLibs.append(FootprintLibrary{fp, Qt::Unchecked, {}});
    }
    for (const FilePath& fp : findItems(dir, QDir::Dirs, "*.3dshapes")) {
      result->package3dLibs.append(Package3DLibrary{fp, {}});
    }
  };

  // Helper to find library files in a directory.
  auto findSymbols = [&](const FilePath& dir) {
    for (const FilePath& fp : findItems(dir, QDir::Files, "*.kicad_sym")) {
      result->symbolLibs.append(SymbolLibrary{fp, Qt::Unchecked, {}});
    }
  };

  // Scan directory for libraries.
  findSymbols(dir);
  findLibs(dir);
  emit progressPercent(20);

  // Scan subdirectories for libraries (not recursive).
  QDir qDir(dir.toStr());
  qDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  foreach (const QFileInfo& info, qDir.entryInfoList()) {
    const FilePath subdir{info.absoluteFilePath()};
    findSymbols(subdir);
    findLibs(subdir);
  }
  emit progressPercent(30);

  // Find footprints & package models.
  for (FootprintLibrary& lib : result->footprintLibs) {
    for (const FilePath& fp : findItems(lib.dir, QDir::Files, "*.kicad_mod")) {
      lib.footprints.append(Footprint{fp, QString(), Qt::Unchecked});
    }
  }
  for (Package3DLibrary& lib : result->package3dLibs) {
    lib.stepFiles += findItems(lib.dir, QDir::Files, "*.step");
  }
  emit progressPercent(50);

  // Helper to get the footprint ID of a symbol.
  auto getFootprintId = [](const KiCadSymbol& symbol) {
    for (const auto& p : symbol.properties) {
      if (p.key == "Footprint") {
        return p.value;
      }
    }
    return QString();
  };

  // Load symbols.
  for (SymbolLibrary& lib : result->symbolLibs) {
    MessageLogger symLog(mLogger.get(), lib.file.getFilename());
    try {
      std::unique_ptr<SExpression> root =
          SExpression::parse(FileUtils::readFile(lib.file), lib.file,
                             SExpression::Mode::Permissive);
      KiCadSymbolLibrary kiLib = KiCadSymbolLibrary::parse(*root, symLog);
      foreach (const auto& symbol, kiLib.symbols) {
        lib.symbols.append(
            Symbol{symbol.name, getFootprintId(symbol), Qt::Unchecked});
      }
    } catch (const Exception& e) {
      symLog.critical(e.getMsg());
    }
  }

  // Load footprints.
  for (FootprintLibrary& lib : result->footprintLibs) {
    for (Footprint& fpt : lib.footprints) {
      MessageLogger fptLog(mLogger.get(), lib.dir.getFilename());
      try {
        std::unique_ptr<SExpression> root =
            SExpression::parse(FileUtils::readFile(fpt.file), fpt.file,
                               SExpression::Mode::Permissive);
        KiCadFootprint kiFpt = KiCadFootprint::parse(*root, fptLog);
        fpt.name = kiFpt.name;
      } catch (const Exception& e) {
        fptLog.critical(e.getMsg());
      }
    }
  }

  // Sort all elements by name to improve readability.
  Toolbox::sortNumeric(
      result->symbolLibs,
      [](const QCollator& cmp, const SymbolLibrary& lhs,
         const SymbolLibrary& rhs) {
        return cmp(lhs.file.getFilename(), rhs.file.getFilename());
      },
      Qt::CaseInsensitive, false);
  Toolbox::sortNumeric(
      result->footprintLibs,
      [](const QCollator& cmp, const FootprintLibrary& lhs,
         const FootprintLibrary& rhs) {
        return cmp(lhs.dir.getFilename(), rhs.dir.getFilename());
      },
      Qt::CaseInsensitive, false);

  emit progressPercent(100);
  return result;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
