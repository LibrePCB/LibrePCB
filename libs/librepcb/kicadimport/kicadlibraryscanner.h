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

#ifndef LIBREPCB_KICADIMPORT_KICADLIBRARYSCANNER_H
#define LIBREPCB_KICADIMPORT_KICADLIBRARYSCANNER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class MessageLogger;

namespace kicadimport {

/*******************************************************************************
 *  Class KiCadLibraryScanner
 ******************************************************************************/

/**
 * @brief KiCad library directory scanner
 */
class KiCadLibraryScanner final : public QObject {
  Q_OBJECT

public:
  struct Result{};

  // Constructors / Destructor
  KiCadLibraryScanner(const KiCadLibraryScanner& other) = delete;
  explicit KiCadLibraryScanner(QObject* parent = nullptr) noexcept;
  ~KiCadLibraryScanner() noexcept;

  // General Methods
  void startScan(const FilePath& fp);
  std::shared_ptr<Result> getResult() noexcept;
  void cancel() noexcept;

  // Operator Overloadings
  KiCadLibraryScanner& operator=(const KiCadLibraryScanner& rhs) = delete;

signals:
  void progressStatus(const QString& status);
  void progressPercent(int percent);
  void finished();

private:
  // Methods
  std::shared_ptr<Result> run(FilePath dir) noexcept;

  // Data
  std::shared_ptr<MessageLogger> mLogger;
  QFuture<std::shared_ptr<Result>> mFuture;

  // State
  bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
