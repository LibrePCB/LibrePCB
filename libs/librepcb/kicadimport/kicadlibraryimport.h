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

#ifndef LIBREPCB_KICADIMPORT_KICADLIBRARYIMPORT_H
#define LIBREPCB_KICADIMPORT_KICADLIBRARYIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class MessageLogger;

namespace kicadimport {

/*******************************************************************************
 *  Class KiCadLibraryImport
 ******************************************************************************/

/**
 * @brief KiCad library import
 */
class KiCadLibraryImport final : public QObject {
  Q_OBJECT

public:
  enum class State {
    Reset,
    Scanning,
    Scanned,
    Parsing,
    Parsed,
    Importing,
    Imported,
  };

  struct Symbol {
    QString name;
    QString footprint;  // LIBNAME:FOOTPRINTNAME (optional)
    Qt::CheckState checkState;
  };

  struct SymbolLibrary {
    FilePath file;
    Qt::CheckState checkState;
    QList<Symbol> symbols;
  };

  struct Footprint {
    FilePath file;
    QString name;
    Qt::CheckState checkState;
  };

  struct FootprintLibrary {
    FilePath dir;
    Qt::CheckState checkState;
    QList<Footprint> footprints;
  };

  struct Package3DLibrary {
    FilePath dir;
    QList<FilePath> stepFiles;
  };

  struct Result {
    QList<SymbolLibrary> symbolLibs;
    QList<FootprintLibrary> footprintLibs;
    QList<Package3DLibrary> package3dLibs;
    int fileCount = 0;
  };

  // Constructors / Destructor
  KiCadLibraryImport(const KiCadLibraryImport& other) = delete;
  KiCadLibraryImport(const FilePath& dstLibFp,
                     QObject* parent = nullptr) noexcept;
  ~KiCadLibraryImport() noexcept;

  // Getters
  const FilePath& getLoadedDirectory() const noexcept {
    return mLoadedDirectory;
  }
  bool canStartParsing() const noexcept;
  bool canStartImport() const noexcept;

  // General Methods
  void reset() noexcept;
  bool startScan(const FilePath& dir,
                 std::shared_ptr<MessageLogger> log) noexcept;
  bool startParse(std::shared_ptr<MessageLogger> log) noexcept;
  bool startImport(std::shared_ptr<MessageLogger> log) noexcept;
  std::shared_ptr<Result> getResult() noexcept;
  void cancel() noexcept;

  // Operator Overloadings
  KiCadLibraryImport& operator=(const KiCadLibraryImport& rhs) = delete;

signals:
  void progressPercent(int percent);
  void scanFinished();
  void parseFinished();
  void importFinished();

private:
  std::shared_ptr<Result> scan(FilePath dir,
                               std::shared_ptr<MessageLogger> log) noexcept;
  std::shared_ptr<Result> parse(std::shared_ptr<Result> result,
                                std::shared_ptr<MessageLogger> log) noexcept;

  const FilePath mDestinationLibraryFp;
  FilePath mLoadedDirectory;
  QFuture<std::shared_ptr<Result>> mFuture;
  State mState;
  bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
