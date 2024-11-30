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
class WorkspaceLibraryDb;

namespace kicadimport {

class KiCadLibraryConverterSettings;

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

  struct Gate {
    int index;  // As specified in KiCad symbol.
    QString symGeneratedBy;  // Symbol "generated_by" property.
    bool alreadyImported;
  };

  struct Symbol {
    QString name;
    QString cmpGeneratedBy;  // Component "generated_by" property.
    QString devGeneratedBy;  // Device "generated_by" property.
    QString pkgGeneratedBy;  // Package "generated_by" property (optional).
    bool symAlreadyImported;
    bool cmpAlreadyImported;
    bool devAlreadyImported;
    QString extends;
    QList<Gate> gates;
    Qt::CheckState symChecked;
    Qt::CheckState cmpChecked;
    Qt::CheckState devChecked;
  };

  struct SymbolLibrary {
    FilePath file;
    QList<Symbol> symbols;
  };

  struct Footprint {
    FilePath file;
    QString name;
    QString generatedBy;  // To be set as "generated_by" property.
    bool alreadyImported;
    Qt::CheckState checked;
  };

  struct FootprintLibrary {
    FilePath dir;
    QList<FilePath> files;
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
  KiCadLibraryImport(WorkspaceLibraryDb& db, const FilePath& dstLibFp,
                     QObject* parent = nullptr) noexcept;
  ~KiCadLibraryImport() noexcept;

  // Getters
  State getState() const noexcept { return mState; }
  const FilePath& getLoadedLibsPath() const noexcept { return mLoadedLibsFp; }
  const FilePath& getLoadedShapes3dPath() const noexcept {
    return mLoadedShapes3dFp;
  }
  bool canStartParsing() const noexcept;
  bool canStartSelecting() const noexcept;
  bool canStartImport() const noexcept;

  // Setters
  void setNamePrefix(const QString& prefix) noexcept;
  void setSymbolCategories(const QSet<Uuid>& uuids) noexcept;
  void setPackageCategories(const QSet<Uuid>& uuids) noexcept;
  void setComponentCategories(const QSet<Uuid>& uuids) noexcept;
  void setDeviceCategories(const QSet<Uuid>& uuids) noexcept;
  void setSymbolChecked(const QString& libName, const QString& symName,
                        bool checked) noexcept;
  void setPackageChecked(const QString& libName, const QString& fptName,
                         bool checked) noexcept;
  void setComponentChecked(const QString& libName, const QString& symName,
                           bool checked) noexcept;
  void setDeviceChecked(const QString& libName, const QString& symName,
                        bool checked) noexcept;

  // General Methods
  void reset() noexcept;
  bool startScan(const FilePath& libsFp, const FilePath& shapes3dFp,
                 std::shared_ptr<MessageLogger> log) noexcept;
  bool startParse(std::shared_ptr<MessageLogger> log) noexcept;
  bool startImport(std::shared_ptr<MessageLogger> log) noexcept;
  bool isRunning() const noexcept;
  std::shared_ptr<Result> getResult() noexcept;
  void cancel() noexcept;

  // Operator Overloadings
  KiCadLibraryImport& operator=(const KiCadLibraryImport& rhs) = delete;

signals:
  void symbolCheckStateChanged(const QString& libName, const QString& symName,
                               Qt::CheckState state);
  void packageCheckStateChanged(const QString& libName, const QString& fptName,
                                Qt::CheckState state);
  void componentCheckStateChanged(const QString& libName,
                                  const QString& symName, Qt::CheckState state);
  void progressPercent(int percent);
  void progressStatus(QString status);
  void scanFinished();
  void parseFinished();
  void importFinished();

private:
  std::shared_ptr<Result> scan(FilePath libsFp, const FilePath& shapes3dFp,
                               std::shared_ptr<MessageLogger> log) noexcept;
  std::shared_ptr<Result> parse(std::shared_ptr<Result> result,
                                std::shared_ptr<MessageLogger> log) noexcept;
  std::shared_ptr<Result> import(std::shared_ptr<Result> result,
                                 std::shared_ptr<MessageLogger> log) noexcept;
  template <typename T>
  bool isAlreadyImported(const QString& generatedBy) const noexcept;
  void updateDependencies(std::shared_ptr<Result> result) noexcept;

  const FilePath mDestinationLibraryFp;
  WorkspaceLibraryDb& mLibraryDb;
  QScopedPointer<KiCadLibraryConverterSettings> mSettings;
  FilePath mLoadedLibsFp;
  FilePath mLoadedShapes3dFp;
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
