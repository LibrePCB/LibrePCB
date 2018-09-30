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
#include "networkaccessmanager.h"

#include "../exceptions.h"

#include <QtCore>
#include <QtNetwork>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Variables
 ******************************************************************************/
NetworkAccessManager* NetworkAccessManager::sInstance = nullptr;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetworkAccessManager::NetworkAccessManager() noexcept
  : QThread(nullptr), mThreadStartSemaphore(0), mManager(nullptr) {
  // This thread must only be started once, and from within the main application
  // thread!
  Q_ASSERT(QThread::currentThread() == qApp->thread());
  Q_ASSERT(sInstance == nullptr);
  sInstance = this;

  // ensure that this thread gets stopped *before* the main thread stops
  connect(qApp, &QCoreApplication::aboutToQuit, this,
          &NetworkAccessManager::stop, Qt::DirectConnection);

  // start the thread and wait until the thread is started successfully
  start();
  mThreadStartSemaphore.acquire();
}

NetworkAccessManager::~NetworkAccessManager() noexcept {
  Q_ASSERT(QThread::currentThread() == qApp->thread());
  stop();  // blocks until the thread has stopped
  sInstance = nullptr;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QNetworkReply* NetworkAccessManager::get(
    const QNetworkRequest& request) noexcept {
  Q_ASSERT(QThread::currentThread() == this);

  if (mManager) {
    return mManager->get(request);
  } else {
    qCritical() << "No network access manager available! Thread not running?!";
    return nullptr;
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

NetworkAccessManager* NetworkAccessManager::instance() noexcept {
  return sInstance;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetworkAccessManager::run() noexcept {
  Q_ASSERT(QThread::currentThread() == this);
  qDebug() << "Started network access manager thread.";
  mManager = new QNetworkAccessManager();
  mThreadStartSemaphore.release();
  try {
    exec();  // event loop (blocking)
  } catch (...) {
    qCritical() << "Exception thrown in network access manager event loop!!!";
    // do NOT exit the thread to avoid further problems due to deleted
    // QNetworkAccessManager
    while (true) {
      QThread::sleep(ULONG_MAX);
    }
  }
  delete mManager;
  mManager = nullptr;
  qDebug() << "Stopped network access manager thread.";
}

void NetworkAccessManager::stop() noexcept {
  Q_ASSERT(QThread::currentThread() != this);
  quit();
  if (!wait(5000)) {
    qWarning() << "Could not quit the network access manager thread!";
    terminate();
    if (!wait(1000)) {
      qCritical() << "Could not terminate the network access manager thread!";
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
