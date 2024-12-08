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
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace kicadimport {

static QString generatedBy(QString libName, QStringList keys) {
  keys.prepend(libName);
  keys.prepend("KiCadImport");
  return keys.join("::");
}

static void mergeSymbolGates(KiCadSymbolGate& out, const KiCadSymbolGate& in) {
  out.arcs += in.arcs;
  out.circles += in.circles;
  out.rectangles += in.rectangles;
  out.polylines += in.polylines;
  out.pins += in.pins;
}

static QList<KiCadSymbolGate> mergeSymbolGates(
    const QList<KiCadSymbolGate>& gates, const QString& symbolName) {
  // Collect all gates.
  QMap<int, KiCadSymbolGate> map;
  for (const KiCadSymbolGate& gate : gates) {
    if ((gate.style == KiCadSymbolGate::Style::Base) ||
        (gate.style == KiCadSymbolGate::Style::Common)) {
      auto it = map.find(gate.index);
      if (it != map.end()) {
        mergeSymbolGates(*it, gate);
      } else {
        map.insert(gate.index, gate);
      }
    }
  }

  if (map.isEmpty()) {
    return QList<KiCadSymbolGate>();
  }

  // Only if we have multiple gates, apply common geometry to other gates
  // (confusing KiCad logic).
  auto commonIt = map.find(0);
  if ((commonIt != map.end()) && (map.count() > 1)) {
    for (auto it = map.begin(); it != map.end(); it++) {
      if (it != commonIt) {
        mergeSymbolGates(*it, *commonIt);
      }
    }
    map.remove(0);
  }

  // Update gate properties.
  QList<KiCadSymbolGate> ret = map.values();
  for (KiCadSymbolGate& gate : ret) {
    gate.name = symbolName;
    if (ret.count() > 1) {
      gate.name += ":" % QString::number(gate.index);
    }
    gate.style = KiCadSymbolGate::Style::Base;
  }
  return ret;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryImport::KiCadLibraryImport(WorkspaceLibraryDb& db,
                                       const FilePath& dstLibFp,
                                       QObject* parent) noexcept
  : QObject(parent),
    mDestinationLibraryFp(dstLibFp),
    mLibraryDb(db),
    mSettings(new KiCadLibraryConverterSettings()),
    mState(State::Reset),
    mAbort(false) {
}

KiCadLibraryImport::~KiCadLibraryImport() noexcept {
  cancel();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool KiCadLibraryImport::canStartParsing() const noexcept {
  if ((mState != State::Scanned) && (mState != State::Parsed) &&
      (mState != State::Imported)) {
    return false;
  }

  auto result = mFuture.result();
  return (result && (result->fileCount > 0));
}

bool KiCadLibraryImport::canStartSelecting() const noexcept {
  if ((mState != State::Parsed) && (mState != State::Imported)) {
    return false;
  }

  auto result = mFuture.result();
  if (!result) {
    return false;
  }

  for (const SymbolLibrary& lib : result->symbolLibs) {
    if (!lib.symbols.isEmpty()) return true;
  }
  for (const FootprintLibrary& lib : result->footprintLibs) {
    if (!lib.footprints.isEmpty()) return true;
  }
  return false;
}

bool KiCadLibraryImport::canStartImport() const noexcept {
  if ((mState != State::Parsed) && (mState != State::Imported)) {
    return false;
  }

  auto result = mFuture.result();
  if (!result) {
    return false;
  }

  for (const SymbolLibrary& lib : result->symbolLibs) {
    for (const Symbol& sym : lib.symbols) {
      if ((sym.symChecked != Qt::Unchecked) && (!sym.symAlreadyImported)) {
        for (const Gate& gate : sym.gates) {
          if (!gate.alreadyImported) {
            return true;
          }
        }
      }
      if ((sym.cmpChecked != Qt::Unchecked) && (!sym.cmpAlreadyImported) &&
          (sym.extends.isEmpty())) {
        return true;
      }
      if ((sym.devChecked != Qt::Unchecked) && (!sym.devAlreadyImported) &&
          (!sym.pkgGeneratedBy.isEmpty())) {
        return true;
      }
    }
  }
  for (const FootprintLibrary& lib : result->footprintLibs) {
    for (const Footprint& fpt : lib.footprints) {
      if ((fpt.checked != Qt::Unchecked) && (!fpt.alreadyImported)) {
        return true;
      }
    }
  }
  return false;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void KiCadLibraryImport::setNamePrefix(const QString& prefix) noexcept {
  mSettings->namePrefix = prefix;
}

void KiCadLibraryImport::setSymbolCategories(const QSet<Uuid>& uuids) noexcept {
  mSettings->symbolCategories = uuids;
}

void KiCadLibraryImport::setPackageCategories(
    const QSet<Uuid>& uuids) noexcept {
  mSettings->packageCategories = uuids;
}

void KiCadLibraryImport::setComponentCategories(
    const QSet<Uuid>& uuids) noexcept {
  mSettings->componentCategories = uuids;
}

void KiCadLibraryImport::setDeviceCategories(const QSet<Uuid>& uuids) noexcept {
  mSettings->deviceCategories = uuids;
}

void KiCadLibraryImport::setSymbolChecked(const QString& libName,
                                          const QString& symName,
                                          bool checked) noexcept {
  std::shared_ptr<Result> result = getResult();
  if (!result) return;

  bool modified = false;
  const Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;
  for (SymbolLibrary& lib : result->symbolLibs) {
    if (lib.file.getCompleteBasename() == libName) {
      for (Symbol& sym : lib.symbols) {
        if ((sym.name == symName) && (sym.symChecked != checkState)) {
          sym.symChecked = checkState;
          modified = true;
        }
      }
    }
  }
  if (modified) {
    updateDependencies(result);
  }
}

void KiCadLibraryImport::setPackageChecked(const QString& libName,
                                           const QString& fptName,
                                           bool checked) noexcept {
  std::shared_ptr<Result> result = getResult();
  if (!result) return;

  bool modified = false;
  const Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;
  for (FootprintLibrary& lib : result->footprintLibs) {
    if (lib.dir.getCompleteBasename() == libName) {
      for (Footprint& fpt : lib.footprints) {
        if ((fpt.name == fptName) && (fpt.checked != checkState)) {
          fpt.checked = checkState;
          modified = true;
        }
      }
    }
  }
  if (modified) {
    updateDependencies(result);
  }
}

void KiCadLibraryImport::setComponentChecked(const QString& libName,
                                             const QString& symName,
                                             bool checked) noexcept {
  std::shared_ptr<Result> result = getResult();
  if (!result) return;

  bool modified = false;
  const Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;
  for (SymbolLibrary& lib : result->symbolLibs) {
    if (lib.file.getCompleteBasename() == libName) {
      for (Symbol& sym : lib.symbols) {
        if ((sym.name == symName) && (sym.cmpChecked != checkState)) {
          sym.cmpChecked = checkState;
          modified = true;
        }
      }
    }
  }
  if (modified) {
    updateDependencies(result);
  }
}

void KiCadLibraryImport::setDeviceChecked(const QString& libName,
                                          const QString& symName,
                                          bool checked) noexcept {
  std::shared_ptr<Result> result = getResult();
  if (!result) return;

  bool modified = false;
  const Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;
  for (SymbolLibrary& lib : result->symbolLibs) {
    if (lib.file.getCompleteBasename() == libName) {
      for (Symbol& sym : lib.symbols) {
        if ((sym.name == symName) && (sym.devChecked != checkState)) {
          sym.devChecked = checkState;
          modified = true;
        }
      }
    }
  }
  if (modified) {
    updateDependencies(result);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImport::reset() noexcept {
  cancel();
  mState = State::Reset;
  mLoadedLibsFp = FilePath();
  mLoadedShapes3dFp = FilePath();
}

bool KiCadLibraryImport::startScan(
    const FilePath& libsFp, const FilePath& shapes3dFp,
    std::shared_ptr<MessageLogger> log) noexcept {
  if (mState != State::Reset) {
    log->critical("Unexpected state.");
    emit scanFinished();
    return false;
  }

  mAbort = false;
  mState = State::Scanning;
  mLoadedLibsFp = libsFp;
  mLoadedShapes3dFp = shapes3dFp;
  mFuture = QtConcurrent::run(&KiCadLibraryImport::scan, this, libsFp,
                              shapes3dFp, log);
  return true;
}

bool KiCadLibraryImport::startParse(
    std::shared_ptr<MessageLogger> log) noexcept {
  if (mState != State::Scanned) {
    log->critical("Unexpected state.");
    parseFinished();
    return false;
  }

  mAbort = false;
  mState = State::Parsing;
  mFuture = QtConcurrent::run(&KiCadLibraryImport::parse, this,
                              mFuture.result(), log);
  return true;
}

bool KiCadLibraryImport::startImport(
    std::shared_ptr<MessageLogger> log) noexcept {
  if (mState != State::Parsed) {
    log->critical("Unexpected state.");
    importFinished();
    return false;
  }

  mAbort = false;
  mState = State::Importing;
  mFuture = QtConcurrent::run(&KiCadLibraryImport::import, this,
                              mFuture.result(), log);
  return true;
}

bool KiCadLibraryImport::isRunning() const noexcept {
  return (mFuture.isStarted() || mFuture.isRunning()) &&
      (!mFuture.isFinished()) && (!mFuture.isCanceled());
}

std::shared_ptr<KiCadLibraryImport::Result>
    KiCadLibraryImport::getResult() noexcept {
  mFuture.waitForFinished();
  return mFuture.result();
}

void KiCadLibraryImport::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

std::shared_ptr<KiCadLibraryImport::Result> KiCadLibraryImport::scan(
    FilePath libsFp, const FilePath& shapes3dFp,
    std::shared_ptr<MessageLogger> log) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Searching for KiCad libraries in worker thread...";

  // Helper to find files or directories in a directory.
  auto findItems = [](const FilePath& dir, QDir::Filter filter,
                      const QString& pattern) {
    QList<FilePath> files;
    QDir qDir(dir.toStr());
    qDir.setFilter(filter | QDir::NoDotAndDotDot);
    if (!pattern.isEmpty()) {
      qDir.setNameFilters({pattern});
    }
    qDir.setSorting(QDir::DirsFirst | QDir::Name);
    foreach (const QFileInfo& info, qDir.entryInfoList()) {
      files.append(FilePath{info.absoluteFilePath()});
    }
    return files;
  };

  auto result = std::make_shared<Result>();
  int footprintCount = 0;
  int stepFileCount = 0;

  // Helpers.
  auto addSymbolLib = [&result](const FilePath& fp) {
    result->symbolLibs.append(SymbolLibrary{fp, {}});
    ++(result->fileCount);
  };
  auto findSymbolLibs = [&](const FilePath& fp) {
    for (const FilePath& subFp : findItems(fp, QDir::Files, "*.kicad_sym")) {
      addSymbolLib(subFp);
    }
  };
  auto addFootprintToLib = [&](FootprintLibrary& lib, const FilePath& fp) {
    lib.files.append(fp);
    ++footprintCount;
    ++(result->fileCount);
  };
  auto addFootprintLib = [&](const FilePath& fp) {
    FootprintLibrary lib{fp, {}, {}};
    for (const FilePath& subFp : findItems(fp, QDir::Files, "*.kicad_mod")) {
      addFootprintToLib(lib, subFp);
    }
    result->footprintLibs.append(lib);
  };
  auto findFootprintLibs = [&](const FilePath& fp) {
    for (const FilePath& subFp : findItems(fp, QDir::Dirs, "*.pretty")) {
      addFootprintLib(subFp);
    }
  };
  auto addShapes3dLib = [&](const FilePath& fp) {
    Package3DLibrary lib{fp, {}};
    lib.stepFiles = findItems(fp, QDir::Files, "*.step");
    stepFileCount += lib.stepFiles.count();
    result->fileCount += lib.stepFiles.count();
    result->package3dLibs.append(lib);
  };
  auto findShapes3dLibs = [&](const FilePath& fp) {
    for (const FilePath& subFp : findItems(fp, QDir::Dirs, "*.3dshapes")) {
      addShapes3dLib(subFp);
    }
  };

  // Scan selected libraries.
  const QString suffix = libsFp.getSuffix().toLower();
  if (suffix == "kicad_sym") {
    // Symbol library selected.
    addSymbolLib(libsFp);
  } else if (suffix == "kicad_mod") {
    // Footprint selected.
    if (libsFp.getParentDir().getSuffix().toLower() == "pretty") {
      FootprintLibrary lib{libsFp.getParentDir(), {}, {}};
      addFootprintToLib(lib, libsFp);
      result->footprintLibs.append(lib);
    } else {
      log->critical("Parent directory is not a *.pretty library.");
    }
  } else if (suffix == "pretty") {
    // Footprint library selected.
    addFootprintLib(libsFp);
  } else if (libsFp.isExistingDir()) {
    // Any other directory selected, scan it for content.
    findSymbolLibs(libsFp);
    findFootprintLibs(libsFp);

    // Scan subdirectories for libraries (not recursive).
    foreach (const FilePath& subFp, findItems(libsFp, QDir::Dirs, QString())) {
      findSymbolLibs(subFp);
      findFootprintLibs(subFp);
    }
  }

  // Look for 3D models.
  const FilePath packageModelsFp = shapes3dFp.isValid() ? shapes3dFp : libsFp;
  if (packageModelsFp.getSuffix().toLower() == "3dshapes") {
    addShapes3dLib(packageModelsFp);
  } else {
    findShapes3dLibs(packageModelsFp);
    foreach (const FilePath& subFp,
             findItems(packageModelsFp, QDir::Dirs, QString())) {
      findShapes3dLibs(subFp);
    }
  }

  // Finished! Report status.
  log->info(tr("Found %1 symbol libraries.").arg(result->symbolLibs.count()));
  log->info(tr("Found %1 footprints in %2 libraries.")
                .arg(footprintCount)
                .arg(result->footprintLibs.count()));
  log->info(tr("Found %1 STEP files in %2 libraries.")
                .arg(stepFileCount)
                .arg(result->package3dLibs.count()));

  qDebug() << "Found" << result->fileCount << "KiCad library files in"
           << timer.elapsed() << "ms.";
  mState = mAbort ? State::Reset : State::Scanned;
  emit scanFinished();
  return result;
}

std::shared_ptr<KiCadLibraryImport::Result> KiCadLibraryImport::parse(
    std::shared_ptr<Result> result,
    std::shared_ptr<MessageLogger> log) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  // Wait for workspace library scan to finish because we need up-to-date
  // "generated_by" entries in the database.
  if (mLibraryDb.isScanInProgress()) {
    log->info(tr("Waiting for background library scan to finish..."));
    while (mLibraryDb.isScanInProgress() && (!mAbort)) QThread::msleep(200);
  }

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Parsing KiCad libraries in worker thread...";
  log->info(tr("Parsing libraries..."));
  emit progressPercent(5);

  // Load symbols.
  int i = 0;
  int symbolCount = 0;
  for (SymbolLibrary& lib : result->symbolLibs) {
    lib.symbols.clear();  // Might be a leftover from previous run.
    if (mAbort) break;

    MessageLogger symLog(log.get(), lib.file.getCompleteBasename());
    try {
      std::unique_ptr<SExpression> root =
          SExpression::parse(FileUtils::readFile(lib.file), lib.file,
                             SExpression::Mode::Permissive);
      KiCadSymbolLibrary kiLib = KiCadSymbolLibrary::parse(*root, symLog);
      for (const auto& kiSymbol : kiLib.symbols) {
        const QString cmpGeneratedBy = generatedBy(
            lib.file.getCompleteBasename(),
            {kiSymbol.extends.isEmpty() ? kiSymbol.name : kiSymbol.extends});
        const QString devGeneratedBy =
            generatedBy(lib.file.getCompleteBasename(), {kiSymbol.name});
        const tl::optional<KiCadProperty> footprintProp =
            KiCadTypeConverter::findProperty(kiSymbol.properties, "footprint");
        const QString footprintStr =
            footprintProp ? footprintProp->value.trimmed() : QString();
        const QStringList footprintSplit =
            footprintStr.isEmpty() ? QStringList() : footprintStr.split(":");
        const QString pkgGeneratedBy = footprintSplit.count()
            ? generatedBy(footprintSplit.value(0), footprintSplit.mid(1))
            : QString();
        if ((!kiSymbol.extends.isEmpty()) && (!kiSymbol.gates.isEmpty())) {
          symLog.critical(
              QString("Symbol '%1' extends another symbol and contains gates.")
                  .arg(kiSymbol.name));
          continue;
        } else if (kiSymbol.extends.isEmpty() && kiSymbol.gates.isEmpty()) {
          symLog.critical(QString("Symbol '%1' does not contain any gates.")
                              .arg(kiSymbol.name));
          continue;
        }
        Symbol sym{
            kiSymbol.name,
            cmpGeneratedBy,
            devGeneratedBy,
            pkgGeneratedBy,
            true,  // Might be set to false below.
            isAlreadyImported<librepcb::Component>(cmpGeneratedBy),
            isAlreadyImported<librepcb::Device>(devGeneratedBy),
            kiSymbol.extends,
            {},
            Qt::Checked,
            Qt::Checked,
            Qt::Checked,
        };
        foreach (const KiCadSymbolGate& gate,
                 mergeSymbolGates(kiSymbol.gates, kiSymbol.name)) {
          const QString genBy =
              generatedBy(lib.file.getCompleteBasename(),
                          {kiSymbol.name, QString::number(gate.index)});
          const bool alreadyImported =
              isAlreadyImported<librepcb::Symbol>(genBy);
          if (!alreadyImported) {
            sym.symAlreadyImported = false;
          }
          sym.gates.append(Gate{
              gate.index,
              genBy,
              alreadyImported,
          });
        }
        lib.symbols.append(sym);
        ++symbolCount;
      }
    } catch (const Exception& e) {
      symLog.critical(QString("Failed to parse symbol library '%1':")
                          .arg(lib.file.getFilename()) %
                      " " % e.getMsg());
    }
    emit progressPercent(5 + (45 * (++i) / result->symbolLibs.count()));
  }

  // Load footprints.
  i = 0;
  int footprintCount = 0;
  for (FootprintLibrary& lib : result->footprintLibs) {
    lib.footprints.clear();  // Might be a leftover from previous run.
    for (const FilePath& fptFp : lib.files) {
      if (mAbort) break;

      MessageLogger fptLog(
          log.get(),
          lib.dir.getCompleteBasename() + ":" + fptFp.getCompleteBasename());
      try {
        std::unique_ptr<SExpression> root = SExpression::parse(
            FileUtils::readFile(fptFp), fptFp, SExpression::Mode::Permissive);
        KiCadFootprint kiFpt = KiCadFootprint::parse(*root, fptLog);
        const QString pkgGeneratedBy = generatedBy(
            lib.dir.getCompleteBasename(), {fptFp.getCompleteBasename()});
        lib.footprints.append(Footprint{
            fptFp,
            kiFpt.name,
            pkgGeneratedBy,
            isAlreadyImported<librepcb::Package>(pkgGeneratedBy),
            Qt::Checked,
        });
        ++footprintCount;
      } catch (const Exception& e) {
        fptLog.critical(
            QString("Failed to parse footprint '%1':")
                .arg(lib.dir.getFilename() + ":" + fptFp.getFilename()) %
            " " % e.getMsg());
      }
    }
    emit progressPercent(50 + (45 * (++i) / result->footprintLibs.count()));
  }

  qDebug() << "Parsed all KiCad libraries in" << timer.elapsed() << "ms.";

  if (mAbort) {
    log->info(tr("Aborted."));
    mState = State::Scanned;
  } else {
    log->info(tr("Found %1 symbols and %2 footprints.")
                  .arg(symbolCount)
                  .arg(footprintCount));
    if (symbolCount + footprintCount > 1000) {
      log->warning(
          tr("Due to the large amount of elements, please be patient during "
             "the following steps."));
    }
    log->info(tr("Please review the messages (if any) before continuing."));
    mState = State::Parsed;
  }
  emit progressPercent(100);
  emit parseFinished();
  return result;
}

std::shared_ptr<KiCadLibraryImport::Result> KiCadLibraryImport::import(
    std::shared_ptr<Result> result,
    std::shared_ptr<MessageLogger> log) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Importing KiCad libraries in worker thread...";
  log->info(tr("Importing libraries..."));
  emit progressPercent(5);

  KiCadLibraryConverter converter(mLibraryDb, *mSettings);
  int totalCount = 0;
  int processedCount = 0;
  int importedCount = 0;

  // Calculate total count.
  for (const FootprintLibrary& lib : result->footprintLibs) {
    for (const Footprint& fpt : lib.footprints) {
      if ((fpt.checked != Qt::Unchecked) && (!fpt.alreadyImported)) {
        ++totalCount;  // Package.
      }
    }
  }
  for (const SymbolLibrary& lib : result->symbolLibs) {
    for (const Symbol& sym : lib.symbols) {
      if ((sym.symChecked != Qt::Unchecked) && (!sym.symAlreadyImported)) {
        for (const Gate& gate : sym.gates) {
          if (!gate.alreadyImported) {
            ++totalCount;  // Symbol.
          }
        }
      }
      if ((sym.cmpChecked != Qt::Unchecked) && (!sym.cmpAlreadyImported) &&
          (sym.extends.isEmpty())) {
        ++totalCount;  // Component.
      }
      if ((sym.devChecked != Qt::Unchecked) && (!sym.devAlreadyImported) &&
          (!sym.pkgGeneratedBy.isEmpty())) {
        ++totalCount;  // Device.
      }
    }
  }

  // Import packages.
  QSet<QString> missing3dShapeLibs;
  for (const FootprintLibrary& lib : result->footprintLibs) {
    for (const Footprint& fpt : lib.footprints) {
      if (mAbort) {
        break;
      }
      if ((fpt.checked == Qt::Unchecked) || fpt.alreadyImported) {
        continue;
      }
      MessageLogger fptLog(
          log.get(),
          lib.dir.getCompleteBasename() % ":" % fpt.file.getCompleteBasename());
      emit progressStatus(lib.dir.getCompleteBasename() % ":" %
                          fpt.file.getCompleteBasename());
      try {
        std::unique_ptr<SExpression> root =
            SExpression::parse(FileUtils::readFile(fpt.file), fpt.file,
                               SExpression::Mode::Permissive);
        KiCadFootprint kiFpt = KiCadFootprint::parse(*root, fptLog);

        // Find 3D models.
        QMap<QString, FilePath> models;
        for (const KiCadFootprintModel& model : kiFpt.models) {
          const QStringList pathSegments = model.path.split("/");
          const QString libName = pathSegments.value(pathSegments.count() - 2);
          const QString fileName = pathSegments.value(pathSegments.count() - 1)
                                       .replace(".wrl", ".step");
          if ((!libName.endsWith(".3dshapes")) ||
              (!fileName.endsWith(".step"))) {
            fptLog.warning(
                QString("Unknown 3D model file: '%1'").arg(model.path));
            continue;
          }
          bool libFound = false;
          for (const Package3DLibrary& lib : result->package3dLibs) {
            if (lib.dir.getFilename() == libName) {
              libFound = true;
              for (const FilePath& fp : lib.stepFiles) {
                if (fp.getFilename() == fileName) {
                  models.insert(model.path, fp);
                }
              }
            }
          }
          if (!libFound) {
            missing3dShapeLibs.insert(libName);
          }
        }

        // Create package.
        auto package =
            converter.createPackage(lib.dir, kiFpt, fpt.generatedBy, models,
                                    fptLog);  // can throw
        TransactionalDirectory dir(TransactionalFileSystem::openRW(
            mDestinationLibraryFp
                .getPathTo(librepcb::Package::getShortElementName())
                .getPathTo(package->getUuid().toStr())));
        package->saveTo(dir);
        dir.getFileSystem()->save();
        ++importedCount;
      } catch (const Exception& e) {
        fptLog.critical(
            tr("Skipped footprint due to error: %1").arg(e.getMsg()));
      }
      ++processedCount;
      emit progressPercent((100 * processedCount) / std::max(totalCount, 1));
    }
  }

  // Import symbols, components & devices.
  for (const SymbolLibrary& lib : result->symbolLibs) {
    MessageLogger libLog(log.get(), lib.file.getCompleteBasename());
    try {
      std::unique_ptr<SExpression> root =
          SExpression::parse(FileUtils::readFile(lib.file), lib.file,
                             SExpression::Mode::Permissive);
      KiCadSymbolLibrary kiLib = KiCadSymbolLibrary::parse(*root, libLog);
      if (lib.symbols.count() != kiLib.symbols.count()) {
        throw LogicError(__FILE__, __LINE__);
      }
      for (int iSym = 0; iSym < lib.symbols.count(); ++iSym) {
        const Symbol& sym = lib.symbols.at(iSym);
        const KiCadSymbol& kiSym = kiLib.symbols.at(iSym);
        MessageLogger symLog(&libLog, kiSym.name);
        QList<KiCadSymbolGate> kiGates =
            mergeSymbolGates(kiSym.gates, kiSym.name);
        if (sym.gates.count() != kiGates.count()) {
          throw LogicError(__FILE__, __LINE__);
        }

        // Import gates as symbols.
        for (int iGate = 0; iGate < sym.gates.count(); ++iGate) {
          if (mAbort) {
            break;
          }
          const Gate& gate = sym.gates.at(iGate);
          const KiCadSymbolGate& kiGate = kiGates.at(iGate);
          if ((sym.symChecked == Qt::Unchecked) || gate.alreadyImported ||
              (!kiSym.extends.isEmpty())) {
            continue;
          }
          MessageLogger gateLog(&symLog, QString::number(kiGate.index));
          emit progressStatus(lib.file.getCompleteBasename() % ":" %
                              kiGate.name);
          try {
            auto symbol = converter.createSymbol(lib.file, kiSym, kiGate,
                                                 gate.symGeneratedBy,
                                                 gateLog);  // can throw
            TransactionalDirectory dir(TransactionalFileSystem::openRW(
                mDestinationLibraryFp
                    .getPathTo(librepcb::Symbol::getShortElementName())
                    .getPathTo(symbol->getUuid().toStr())));
            symbol->saveTo(dir);
            dir.getFileSystem()->save();
            ++importedCount;
          } catch (const Exception& e) {
            gateLog.critical(
                tr("Skipped symbol due to error: %1").arg(e.getMsg()));
          }
          ++processedCount;
          emit progressPercent((100 * processedCount) /
                               std::max(totalCount, 1));
        }

        // Import symbol as component, if it's not extending another symbol.
        if ((sym.cmpChecked != Qt::Unchecked) && (!sym.cmpAlreadyImported) &&
            (kiSym.extends.isEmpty())) {
          if (mAbort) {
            break;
          }
          emit progressStatus(lib.file.getCompleteBasename() % ":" %
                              kiSym.name % ":CMP");
          try {
            QStringList symGeneratedBy;
            for (const Gate& gate : sym.gates) {
              symGeneratedBy.append(gate.symGeneratedBy);
            }
            auto component = converter.createComponent(
                lib.file, kiSym, kiGates, sym.cmpGeneratedBy, symGeneratedBy,
                symLog);  // can throw
            TransactionalDirectory dir(TransactionalFileSystem::openRW(
                mDestinationLibraryFp
                    .getPathTo(librepcb::Component::getShortElementName())
                    .getPathTo(component->getUuid().toStr())));
            component->saveTo(dir);
            dir.getFileSystem()->save();
            ++importedCount;
          } catch (const Exception& e) {
            symLog.critical(
                tr("Skipped component due to error: %1").arg(e.getMsg()));
          }
          ++processedCount;
          emit progressPercent((100 * processedCount) /
                               std::max(totalCount, 1));
        }

        // Import symbol as device.
        if ((sym.devChecked != Qt::Unchecked) && (!sym.devAlreadyImported) &&
            (!sym.pkgGeneratedBy.isEmpty())) {
          if (mAbort) {
            break;
          }
          emit progressStatus(lib.file.getCompleteBasename() % ":" %
                              kiSym.name % ":DEV");
          try {
            bool baseSymbolFound = true;
            if (!kiSym.extends.isEmpty()) {
              for (const KiCadSymbol& kiSymBase : kiLib.symbols) {
                if (kiSymBase.name == kiSym.extends) {
                  kiGates = mergeSymbolGates(kiSymBase.gates, kiSymBase.name);
                  baseSymbolFound = true;
                }
              }
            }
            if (!baseSymbolFound) {
              throw RuntimeError(
                  __FILE__, __LINE__,
                  QString("Base symbol '%1' not found.").arg(kiSym.extends));
            }
            auto device = converter.createDevice(
                lib.file, kiSym, kiGates, sym.devGeneratedBy,
                sym.cmpGeneratedBy, sym.pkgGeneratedBy,
                symLog);  // can throw
            TransactionalDirectory dir(TransactionalFileSystem::openRW(
                mDestinationLibraryFp
                    .getPathTo(librepcb::Device::getShortElementName())
                    .getPathTo(device->getUuid().toStr())));
            device->saveTo(dir);
            dir.getFileSystem()->save();
            ++importedCount;
          } catch (const Exception& e) {
            symLog.critical(
                tr("Skipped device due to error: %1").arg(e.getMsg()));
          }
          ++processedCount;
          emit progressPercent((100 * processedCount) /
                               std::max(totalCount, 1));
        }
      }
    } catch (const Exception& e) {
      libLog.critical(
          tr("Skipped symbol library due to error: %1").arg(e.getMsg()));
    }
  }

  // Warn about missing 3D shape libraries.
  foreach (const QString& libName, Toolbox::sortedQSet(missing3dShapeLibs)) {
    log->info(QString("3D model library not found: '%1'").arg(libName));
  }

  qDebug() << "Imported all KiCad libraries in" << timer.elapsed() << "ms.";

  if (mAbort) {
    log->info(tr("Aborted."));
    mState = State::Scanned;  // Parse result not valid anymore!
  } else {
    log->info(
        tr("Done! Please check all messages (if any) before proceeding."));
    log->info(
        tr("Note that the importer might not cover all cases correctly yet."));
    log->info(tr("If you experience any issue, please <a href=\"%1\">let us "
                 "know</a>. Thanks!")
                  .arg("https://librepcb.org/help/"));
    mState = State::Imported;
  }
  emit progressPercent(100);
  emit progressStatus(tr("Finished: %1 of %2 element(s) imported",
                         "Placeholders are numbers", totalCount)
                          .arg(importedCount)
                          .arg(totalCount));
  emit importFinished();
  return result;
}

template <typename T>
bool KiCadLibraryImport::isAlreadyImported(
    const QString& generatedBy) const noexcept {
  try {
    const QSet<Uuid> uuids = mLibraryDb.getGenerated<T>(generatedBy);
    return !uuids.isEmpty();
  } catch (const Exception& e) {
    qCritical() << "Failed to get imported filepath:" << e.getMsg();
  }
  return false;
}

static bool setDependent(bool dependent, Qt::CheckState& checkState) noexcept {
  if (dependent && (checkState == Qt::Unchecked)) {
    checkState = Qt::PartiallyChecked;
    return true;
  } else if ((!dependent) && (checkState == Qt::PartiallyChecked)) {
    checkState = Qt::Unchecked;
    return true;
  }
  return false;
}

void KiCadLibraryImport::updateDependencies(
    std::shared_ptr<Result> result) noexcept {
  QSet<QString> dependentPackages;
  QSet<QString> dependentComponents;
  for (SymbolLibrary& lib : result->symbolLibs) {
    for (Symbol& sym : lib.symbols) {
      if ((sym.devChecked != Qt::Unchecked) && (!sym.devAlreadyImported) &&
          (!sym.pkgGeneratedBy.isEmpty())) {
        dependentComponents.insert(sym.cmpGeneratedBy);
        dependentPackages.insert(sym.pkgGeneratedBy);
      }
    }
  }

  for (SymbolLibrary& lib : result->symbolLibs) {
    for (Symbol& sym : lib.symbols) {
      if (sym.extends.isEmpty()) {
        if (setDependent(dependentComponents.contains(sym.cmpGeneratedBy),
                         sym.cmpChecked)) {
          emit componentCheckStateChanged(lib.file.getCompleteBasename(),
                                          sym.name, sym.cmpChecked);
        }
        if (setDependent(sym.cmpChecked != Qt::Unchecked, sym.symChecked)) {
          emit symbolCheckStateChanged(lib.file.getCompleteBasename(), sym.name,
                                       sym.symChecked);
        }
      }
    }
  }

  for (FootprintLibrary& lib : result->footprintLibs) {
    for (Footprint& fpt : lib.footprints) {
      if (setDependent(dependentPackages.contains(fpt.generatedBy),
                       fpt.checked)) {
        emit packageCheckStateChanged(lib.dir.getCompleteBasename(), fpt.name,
                                      fpt.checked);
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb
