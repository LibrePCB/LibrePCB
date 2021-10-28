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

#ifndef LIBREPCB_NETWORKACCESSMANAGER_H
#define LIBREPCB_NETWORKACCESSMANAGER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"

#include <QtCore>
#include <QtNetwork>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class NetworkAccessManager
 ******************************************************************************/

/**
 * @brief A network access manager which processes network requests in a
 separate thread
 *
 * @note    One instance of this class must be created in the main application
 thread, and
 *          must be deleted before stopping the main application thread. It's
 not allowed
 *          to create a librepcb::NetworkAccessManager object in other threads,
 or to
 *          create multiple instances at the same time.
 *
 * After the singleton was created, you can get it with the static method
 #instance().
 * But for executing network requests, you don't need to access this object
 directly.
 * You only need the classes librepcb::NetworkRequest and librepcb::FileDownload
 instead.

 * @see librepcb::NetworkRequestBase, librepcb::NetworkRequest,
 librepcb::FileDownload
 */
class NetworkAccessManager final : public QThread {
  Q_OBJECT

public:
  // Constructors / Destructor
  NetworkAccessManager() noexcept;
  NetworkAccessManager(const NetworkAccessManager& other) = delete;
  ~NetworkAccessManager() noexcept;

  // General Methods
  QNetworkReply* get(const QNetworkRequest& request) noexcept;
  QNetworkReply* post(const QNetworkRequest& request,
                      const QByteArray& data) noexcept;

  // Operator Overloadings
  NetworkAccessManager& operator=(const NetworkAccessManager& rhs) = delete;

  // Static Methods
  static NetworkAccessManager* instance() noexcept;

private:  // Methods
  void run() noexcept override;
  void stop() noexcept;

private:  // Data
  QSemaphore mThreadStartSemaphore;
  QNetworkAccessManager* mManager;
  static NetworkAccessManager* sInstance;
};

}  // namespace librepcb

#endif  // LIBREPCB_NETWORKACCESSMANAGER_H
