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

#ifndef LIBREPCB_CORE_ASYNCCOPYOPERATION_H
#define LIBREPCB_CORE_ASYNCCOPYOPERATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AsyncCopyOperation
 ******************************************************************************/

/**
 * @brief High-level helper class to asynchronously and recursively copy
 *        directories with progress indicator
 */
class AsyncCopyOperation final : public QThread {
  Q_OBJECT

public:
  // Constructors / Destructor
  AsyncCopyOperation() = delete;
  AsyncCopyOperation(const AsyncCopyOperation& other) = delete;
  AsyncCopyOperation(const FilePath& source, const FilePath& destination,
                     QObject* parent = nullptr) noexcept;
  ~AsyncCopyOperation() noexcept;

  // Getters
  const FilePath& getSource() const noexcept { return mSource; }
  const FilePath& getDestination() const noexcept { return mDestination; }

  // General Methods

  /**
   * @brief Abort a running copy operation
   *
   * If the copy operation was not finished yet, it is completely reverted,
   * i.e. the destination will be deleted to revert to the state as before
   * starting the copy operation.
   *
   * @note  This calls QThread::wait() so the calling thread blocks until the
   *        operation is completely aborted. If no copy operation is running,
   *        this method has no effect.
   */
  void abort() noexcept;

  // Operator Overloadings
  AsyncCopyOperation& operator=(const AsyncCopyOperation& rhs) = delete;

signals:
  void started();
  void progressStatus(const QString& status);
  void progressPercent(int percent);
  void succeeded();
  void failed(const QString& error);
  void finished();

private:  // Methods
  void run() noexcept override;

private:  // Data
  FilePath mSource;
  FilePath mDestination;
  volatile bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
