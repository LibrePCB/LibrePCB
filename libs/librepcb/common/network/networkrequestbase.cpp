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
#include "networkrequestbase.h"

#include "../application.h"
#include "networkaccessmanager.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetworkRequestBase::NetworkRequestBase(const QUrl& url) noexcept
  : mUrl(url),
    mExpectedContentSize(-1),
    mStarted(false),
    mAborted(false),
    mErrored(false),
    mFinished(false) {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());

  // set initial HTTP header fields
  QString userAgent = QString("LibrePCB/%1").arg(qApp->applicationVersion());
  mRequest.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
  mRequest.setRawHeader("X-LibrePCB-AppVersion",
                        qApp->applicationVersion().toUtf8());
  mRequest.setRawHeader("X-LibrePCB-GitRevision",
                        qApp->getGitRevision().toUtf8());
  mRequest.setRawHeader("X-LibrePCB-FileFormatVersion",
                        qApp->getFileFormatVersion().toStr().toUtf8());

  // create queued connection to let executeRequest() execute in download thread
  connect(this, &NetworkRequestBase::startRequested, this,
          &NetworkRequestBase::executeRequest, Qt::QueuedConnection);
}

NetworkRequestBase::~NetworkRequestBase() noexcept {
  if (mStarted) {
    Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  } else {
    Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  }
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void NetworkRequestBase::setHeaderField(const QByteArray& name,
                                        const QByteArray& value) noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mRequest.setRawHeader(name, value);
}

void NetworkRequestBase::setExpectedReplyContentSize(qint64 bytes) noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mExpectedContentSize = bytes;
}

void NetworkRequestBase::start() noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());

  NetworkAccessManager* nam = NetworkAccessManager::instance();
  if (nam) {
    mStarted = true;
    moveToThread(
        nam);  // move event processing of this object to the download thread
    emit progressState(tr("Start request..."));
    emit startRequested();  // execute executeRequest() in download thread
  } else {
    finalize(tr("Fatal error: Download manager is not running."));
  }
}

void NetworkRequestBase::abort() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  if (mReply) {
    emit progressState(tr("Abort request..."));
    mAborted = true;
    mReply->abort();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetworkRequestBase::executeRequest() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());

  emit progressState(tr("Request started..."));

  // get network access manager object
  NetworkAccessManager* nam = NetworkAccessManager::instance();
  if (!nam) {
    finalize(tr("Network access manager is not running."));
    return;
  }

  // prepare request
  try {
    prepareRequest();  // can throw
  } catch (const Exception& e) {
    finalize(e.getMsg());
    return;
  }

  // start request
  mRequest.setUrl(mUrl);
  mReply.reset(nam->get(mRequest));
  if (mReply.isNull()) {
    finalize(
        tr("GET request failed! Network access manager thread not running?!"));
    return;
  }

  // connect to signals of reply
  connect(mReply.data(), &QNetworkReply::readyRead, this,
          &NetworkRequestBase::replyReadyReadSlot);
  connect(mReply.data(),
          static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(
              &QNetworkReply::error),
          this, &NetworkRequestBase::replyErrorSlot);
  connect(mReply.data(), &QNetworkReply::sslErrors, this,
          &NetworkRequestBase::replySslErrorsSlot);
  connect(mReply.data(), &QNetworkReply::downloadProgress, this,
          &NetworkRequestBase::replyDownloadProgressSlot);
  connect(mReply.data(), &QNetworkReply::finished, this,
          &NetworkRequestBase::replyFinishedSlot);
}

void NetworkRequestBase::replyReadyReadSlot() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  fetchNewData();
}

void NetworkRequestBase::replyErrorSlot(
    QNetworkReply::NetworkError code) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  mErrored = true;
  finalize(QString(tr("%1 (%2)")).arg(mReply->errorString()).arg(code));
}

void NetworkRequestBase::replySslErrorsSlot(
    const QList<QSslError>& errors) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  mErrored = true;
  QStringList errorsList;
  foreach (const QSslError& e, errors) { errorsList.append(e.errorString()); }
  finalize(QString(tr("SSL errors occured:\n\n%1")).arg(errorsList.join("\n")));
}

void NetworkRequestBase::replyDownloadProgressSlot(qint64 bytesReceived,
                                                   qint64 bytesTotal) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  if (mAborted || mErrored || mFinished) return;
  if (mReply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid())
    return;

  qint64 estimatedTotal = (bytesTotal > 0) ? bytesTotal : mExpectedContentSize;
  if (estimatedTotal < bytesReceived) {
    estimatedTotal = bytesReceived + 10e6;
  }
  int estimatedPercent =
      (100 * bytesReceived) / qMax(estimatedTotal, qint64(1));
  emit progressState(
      QString(tr("Receive data: %1")).arg(formatFileSize(bytesReceived)));
  emit progressPercent(estimatedPercent);
  emit progress(bytesReceived, bytesTotal, estimatedPercent);
}

void NetworkRequestBase::replyFinishedSlot() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());

  // check if an error was already handled
  if (mErrored) {
    return;
  }

  // check if the request was aborted
  if (mAborted) {
    finalize(tr("Network request aborted."));
    return;
  }

  // check if we received a redirection
  QUrl redirectUrl =
      mReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
  if (!redirectUrl.isEmpty()) {
    redirectUrl = mUrl.resolved(redirectUrl);  // handle relative URLs
    if (mRedirectedUrls.contains(redirectUrl)) {
      finalize(tr("Redirection loop detected."));
      return;
    } else if (mRedirectedUrls.count() > 10) {
      finalize(tr("Too many redirects."));
      return;
    } else {
      // follow redirection
      qDebug() << "Redirect from" << mUrl.toString() << "to"
               << redirectUrl.toString();
      emit progressState(
          QString(tr("Redirect to %1...")).arg(redirectUrl.toString()));
      mReply.take()->deleteLater();
      mRedirectedUrls.append(mUrl);
      mUrl = redirectUrl;
      executeRequest();  // restart download with new url
      return;
    }
  }

  // check for download error
  if (mReply->error() != QNetworkReply::NoError) {
    finalize(
        QString(tr("%1 (%2)")).arg(mReply->errorString()).arg(mReply->error()));
    return;
  }

  // finalize download
  try {
    finalizeRequest();  // can throw
  } catch (const Exception& e) {
    finalize(e.getMsg());
    return;
  }

  // download successfully finished!
  finalize();
}

void NetworkRequestBase::finalize(const QString& errorMsg) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());

  if (errorMsg.isNull()) {
    qDebug() << "Request successfully finished:" << mUrl.toString();
    emit progressState(tr("Request successfully finished."));
    emitSuccessfullyFinishedSignals();
    emit succeeded();
    emit finished(true);
  } else if (mAborted) {
    qDebug() << "Request aborted:" << mUrl.toString();
    emit progressState(tr("Request aborted."));
    emit aborted();
    emit finished(false);
  } else {
    qDebug() << "Request failed:" << mUrl.toString();
    qDebug() << "Network error:" << errorMsg;
    emit progressState(QString(tr("Request failed: %1")).arg(errorMsg));
    emit errored(errorMsg);
    emit finished(false);
  }
  mFinished = true;
  deleteLater();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QString NetworkRequestBase::formatFileSize(qint64 bytes) noexcept {
  qreal               num = bytes;
  QStringList         list({"KB", "MB", "GB", "TB"});
  QStringListIterator i(list);
  QString             unit("Bytes");
  while (num >= 1024.0 && i.hasNext()) {
    unit = i.next();
    num /= 1024;
  }
  return QString::number(num, 'f', 2) % " " % unit;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
